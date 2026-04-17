using System.Collections.Generic;
using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif

namespace PJ.Core.Actor
{
    // SMR : SkinnedMeshRenderer

    // (의상/파츠)를 장착한 상태의 캐릭터 프리팹을 만들 때 사용한다.
    // 1. EquipPartsRender에 CharacterPartRigBinder 컴포넌트를 추가한다.
    // 2. characterRigRoot : 캐릭터 프리팹의 본 루트를 가져와 등록한다.
    // 3. sourceSmr : (의상/파츠)의 프리팹이나 fbx에 있는 SMR을 가져와 등록한다.
    // 4. targetSmr : EquipPartsRender에 붙어있는 Top,Bottom등 Renderer를 등록한다.
    // 5. 인스펙터의 CharacterPartRigBinder(스크립트) 마우스 우클릭 메뉴 -> Remap Bones -> 저장

#if UNITY_EDITOR
    // CharacterPartRigBinder는 캐릭터 파츠 구조와 강하게 결합되어 있어
    // Editor 폴더로 분리하지 않고 동일 파일에서 관리한다.
    [Tooltip("(의상/파츠)를 장착한 상태의 캐릭터 프리팹을 만들 때 사용한다.")]
    public class CharacterPartRigBinder : MonoBehaviour
    {
        [Header("원본 의상/파츠 SMR")]
        [SerializeField] private SkinnedMeshRenderer _sourceSmr;

        [Header("캐릭터 실제 RigRoot")]
        [SerializeField] private Transform _characterRigRoot;

        [Header("장착 대상 SMR")]
        [SerializeField] private SkinnedMeshRenderer _targetSmr;

        [ContextMenu("Remap Bones")]
        private void RemapBones()
        {
            bool ok = SkinnedMeshBoneRemapper.RemapByBoneName(
                sourceSmr: _sourceSmr,
                sourceRigRoot: _characterRigRoot,
                targetSmr: _targetSmr,
                copyMeshAndMaterials: true,
                copyLocalBounds: true);

            if (!ok)
            {
                Debug.LogError("[CharacterPartRigBinder] Remap failed.");
                return;
            }

            SavePrefabOrInstance(_targetSmr.gameObject);

            Debug.Log("[CharacterPartRigBinder] Remap success.");
        }

        private void SavePrefabOrInstance(GameObject go)
        {
            // 1) 프리팹 에셋 자체를 수정한 경우
            if (PrefabUtility.IsPartOfPrefabAsset(go))
            {
                EditorUtility.SetDirty(go);
                PrefabUtility.SavePrefabAsset(go);
                AssetDatabase.SaveAssets();
                return;
            }

            // 2) 씬 안의 프리팹 인스턴스를 수정한 경우
            if (PrefabUtility.IsPartOfNonAssetPrefabInstance(go))
            {
                EditorUtility.SetDirty(go);
                PrefabUtility.RecordPrefabInstancePropertyModifications(go);

                GameObject instanceRoot = PrefabUtility.GetOutermostPrefabInstanceRoot(go);
                if (instanceRoot != null)
                {
                    PrefabUtility.ApplyPrefabInstance(
                        instanceRoot,
                        InteractionMode.UserAction);
                }

                AssetDatabase.SaveAssets();
                return;
            }

            // 3) 일반 씬 오브젝트인 경우
            EditorUtility.SetDirty(go);
        }
    }
#endif

    // 캐릭터 Rig의 Transform 하이어라키를 기준으로
    // 의상/파츠 SMR(Skinned Mesh Renderer)의 bones, rootBone을 재매핑한다.
    public static class SkinnedMeshBoneRemapper
    {
        // sourceRigRoot 아래에서 본 이름 기준 매핑을 만들고,
        // sourceSmr의 bones 순서를 유지한 채 targetSmr에 연결한다.
        // 필요하면 sharedMesh, sharedMaterials, localBounds도 함께 복사한다.
        public static bool RemapByBoneName(
            SkinnedMeshRenderer sourceSmr,
            Transform sourceRigRoot,
            SkinnedMeshRenderer targetSmr,
            bool copyMeshAndMaterials = true,
            bool copyLocalBounds = true)
        {
            if (sourceSmr == null)
            {
                Debug.LogError("[SkinnedMeshBoneRemapper] sourceSmr is null.");
                return false;
            }

            if (sourceRigRoot == null)
            {
                Debug.LogError("[SkinnedMeshBoneRemapper] sourceRigRoot is null.");
                return false;
            }

            if (targetSmr == null)
            {
                Debug.LogError("[SkinnedMeshBoneRemapper] targetSmr is null.");
                return false;
            }

            if (sourceSmr.sharedMesh == null)
            {
                Debug.LogError("[SkinnedMeshBoneRemapper] sourceSmr.sharedMesh is null.");
                return false;
            }

            Transform[] sourceBones = sourceSmr.bones;
            if (sourceBones == null || sourceBones.Length == 0)
            {
                Debug.LogError($"[SkinnedMeshBoneRemapper] sourceSmr '{sourceSmr.name}' has no bones.");
                return false;
            }

            Matrix4x4[] bindposes = sourceSmr.sharedMesh.bindposes;
            if (bindposes == null || bindposes.Length != sourceBones.Length)
            {
                Debug.LogError(
                    $"[SkinnedMeshBoneRemapper] bindpose count mismatch. " +
                    $"bones={sourceBones.Length}, bindposes={(bindposes == null ? 0 : bindposes.Length)}");
                return false;
            }

            Dictionary<string, Transform> rigMap = BuildRigMap(sourceRigRoot);

            Transform[] remappedBones = new Transform[sourceBones.Length];

            for (int i = 0; i < sourceBones.Length; i++)
            {
                Transform srcBone = sourceBones[i];
                if (srcBone == null)
                {
                    Debug.LogError($"[SkinnedMeshBoneRemapper] source bone at index {i} is null.");
                    return false;
                }

                if (!rigMap.TryGetValue(srcBone.name, out Transform dstBone) || dstBone == null)
                {
                    Debug.LogError(
                        $"[SkinnedMeshBoneRemapper] Missing bone '{srcBone.name}' in rig root '{sourceRigRoot.name}'.");
                    return false;
                }

                remappedBones[i] = dstBone;
            }

            Transform remappedRootBone = null;

            if (sourceSmr.rootBone != null)
            {
                if (!rigMap.TryGetValue(sourceSmr.rootBone.name, out remappedRootBone) || remappedRootBone == null)
                {
                    Debug.LogError(
                        $"[SkinnedMeshBoneRemapper] Missing rootBone '{sourceSmr.rootBone.name}' in rig root '{sourceRigRoot.name}'.");
                    return false;
                }
            }

            if (copyMeshAndMaterials)
            {
                targetSmr.sharedMesh = sourceSmr.sharedMesh;
                targetSmr.sharedMaterials = sourceSmr.sharedMaterials;
            }

            targetSmr.rootBone = remappedRootBone;
            targetSmr.bones = remappedBones;

            if (copyLocalBounds)
            {
                targetSmr.localBounds = sourceSmr.localBounds;
                targetSmr.updateWhenOffscreen = sourceSmr.updateWhenOffscreen;
            }

            return true;
        }

        // sourceRigRoot 아래 전체 본을 이름으로 찾아 targetSmr에 직접 연결한다.
        public static bool RemapSelfToRig(
            SkinnedMeshRenderer targetSmr,
            Transform sourceRigRoot,
            bool copyLocalBounds = false)
        {
            return RemapByBoneName(
                sourceSmr: targetSmr,
                sourceRigRoot: sourceRigRoot,
                targetSmr: targetSmr,
                copyMeshAndMaterials: false,
                copyLocalBounds: copyLocalBounds);
        }

        // sourceRigRoot 아래 전체 Transform을 이름 기준으로 맵으로 만든다.
        private static Dictionary<string, Transform> BuildRigMap(Transform root)
        {
            Dictionary<string, Transform> map = new Dictionary<string, Transform>(256);
            Transform[] all = root.GetComponentsInChildren<Transform>(true);

            foreach (Transform t in all)
            {
                if (t == null)
                    continue;

                if (map.ContainsKey(t.name))
                {
                    Debug.LogWarning(
                        $"[SkinnedMeshBoneRemapper] Duplicate bone name detected: '{t.name}'. " +
                        $"First match will be used.");
                    continue;
                }

                map.Add(t.name, t);
            }

            return map;
        }
    }
}
