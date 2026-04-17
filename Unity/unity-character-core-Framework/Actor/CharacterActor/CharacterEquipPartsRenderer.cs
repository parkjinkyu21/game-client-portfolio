using System;
using System.Collections.Generic;
using UnityEngine;

namespace PJ.Core.Actor
{
    // 장착 파츠용 SkinnedMeshRenderer를 관리하는 컴포넌트
    // - 장착 부위별 대상 Renderer를 보관한다.
    // - 장착 프리팹의 메시, 머터리얼, 본 정보를 대상 Renderer에 적용한다.
    // - CharacterView와 연결되어 캐릭터 장비의 표시/해제를 담당한다.
    public class CharacterEquipPartsRenderer : MonoBehaviour
    {
        [Serializable]
        private struct EquipPart
        {
            public CharacterEquipPart part;
            public SkinnedMeshRenderer renderer;
        }

        [SerializeField] private EquipPart[] _renderers;
        [SerializeField] private Transform _characterRigRoot;

        private static readonly Material[] EmptyMaterials = Array.Empty<Material>();

        private CharacterView _characterView;
        private Dictionary<CharacterEquipPart, SkinnedMeshRenderer> _rendererMap;

        public void SetCharacterView(CharacterView view) { _characterView = view; }

        private void Awake()
        {
            _rendererMap = new Dictionary<CharacterEquipPart, SkinnedMeshRenderer>();

            foreach (var r in _renderers)
            {
                if (r.renderer != null)
                    _rendererMap[r.part] = r.renderer;
            }
        }

        public SkinnedMeshRenderer GetRenderer(CharacterEquipPart part)
        {
            _rendererMap.TryGetValue(part, out var renderer);
            return renderer;
        }

        public void SetMesh(CharacterEquipPart part, Mesh mesh, Material[] materials)
        {
            var renderer = GetRenderer(part);
            if (renderer == null)
                return;

            renderer.sharedMesh = mesh;
            renderer.materials = materials;
        }

        // 장착 프리팹의 SMR 데이터(메시, 머터리얼, 본)를 대상 Renderer에 적용한다.
        public void Equip(CharacterEquipPart part, GameObject prefabGO)
        {
            var target = GetRenderer(part);
            if (target == null || prefabGO == null)
                return;

            var src = prefabGO.GetComponentInChildren<SkinnedMeshRenderer>();
            if (src == null)
            {
                Debug.LogError($"[EquipPartsRenderer] No SMR in {prefabGO.name}");
                return;
            }

            // 메시, 머터리얼, 본을 Src 로부터 복사해서 Target 에 지정한다.
            bool ok = SkinnedMeshBoneRemapper.RemapByBoneName(
                sourceSmr: src,
                sourceRigRoot: _characterRigRoot,
                targetSmr: target,
                copyMeshAndMaterials: true,
                copyLocalBounds: true);

            if (!ok)
            {
                Debug.LogError($"[EquipPartsRenderer] Bone remap failed for {prefabGO.name}");
                return;
            }

            target.enabled = true;
        }

        public void UnEquip(CharacterEquipPart part)
        {
            var target = GetRenderer(part);
            if (target == null)
                return;

            target.sharedMesh = null;
            target.sharedMaterials = EmptyMaterials;
            target.enabled = false;
        }
    }
}
