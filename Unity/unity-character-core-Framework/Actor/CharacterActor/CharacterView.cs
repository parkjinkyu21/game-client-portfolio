using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

namespace PJ.Core.Actor
{
    // 캐릭터의 표시(View)와 관련된 구성 요소를 관리한다.
    // - 캐릭터의 스켈레톤 루트와 Animator를 보관한다.
    // - 바디 파츠별 SkinnedMeshRenderer를 관리한다.
    // - 바디 파츠별 메시없이 단일 메시도 가능하다.
    // - 장비 장착/해제 시 파츠 프리팹 적용을 CharacterEquipPartsRenderer에 위임한다.
    // - 장비별 숨김 룰(CharacterEquipPartsHideRule)에 의해서 바디 렌더러 표시 상태를 갱신한다.
    public class CharacterView : MonoBehaviour
    {
        [Serializable]
        private struct RendererPart
        {
            public CharacterRenderPart part;
            public SkinnedMeshRenderer renderer;
        }

        // 박싱 회피용 — Enum.GetValues 의 결과를 정적 캐시
        private static readonly CharacterEquipPart[] EquipPartValues =
            (CharacterEquipPart[])Enum.GetValues(typeof(CharacterEquipPart));

        [SerializeField] private Transform _skeletonRoot;
        public Transform SkeletonRoot => _skeletonRoot;

        [SerializeField] private RendererPart[] _renderers;

        [SerializeField] private float _rotateSpeed = 0.2f;

        public Transform Root => transform;
        public Animator Animator { get; private set; }
        public CharacterEquipPartsRenderer EquipPartsRenderer { get; private set; }

        private Dictionary<CharacterRenderPart, SkinnedMeshRenderer> _rendererMap;
        private Dictionary<CharacterEquipPart, List<CharacterRenderPart>> _equipPartsHideRule;
        private HashSet<CharacterRenderPart> _partsHideSet;

        // SetVisible 용 캐시 — 매 호출마다 GetComponentsInChildren 을 피하기 위해 캐시한다.
        private Renderer[] _cachedRenderers;

        private void Awake()
        {
            Animator = GetComponentInChildren<Animator>();
            EquipPartsRenderer = GetComponentInChildren<CharacterEquipPartsRenderer>();
            EquipPartsRenderer?.SetCharacterView(this);

            // rendererMap 설정
            _rendererMap = new Dictionary<CharacterRenderPart, SkinnedMeshRenderer>();
            foreach (var r in _renderers)
                _rendererMap[r.part] = r.renderer;

            // equipPartsHideRule 설정
            _equipPartsHideRule = new Dictionary<CharacterEquipPart, List<CharacterRenderPart>>();
            foreach (CharacterEquipPart equipPart in EquipPartValues)
                _equipPartsHideRule[equipPart] = new List<CharacterRenderPart>();

            _partsHideSet = new HashSet<CharacterRenderPart>();

            RefreshRendererCache();
        }

        // 모든 렌더러의 표시/숨김을 제어한다.
        // visible=true 일 때 장착 숨김 룰을 재적용한다.
        public void SetVisible(bool visible)
        {
            if (_cachedRenderers == null)
                RefreshRendererCache();

            foreach (var r in _cachedRenderers)
            {
                if (r != null)
                    r.enabled = visible;
            }

            if (visible)
                ApplyHideRule();
        }

        // 렌더러 캐시를 재빌드한다. 하이어라키가 변경되었을 때 호출한다.
        public void RefreshRendererCache()
        {
            _cachedRenderers = GetComponentsInChildren<Renderer>(true);
        }

        public SkinnedMeshRenderer GetRendererPart(CharacterRenderPart part)
        {
            _rendererMap.TryGetValue(part, out var r);
            return r;
        }

        public void Equip(CharacterEquipPart part, GameObject prefabGO)
        {
            // 장착 룰 적용
            _equipPartsHideRule[part].Clear();
            CharacterEquipPartsHideRule hideRule = prefabGO.GetComponent<CharacterEquipPartsHideRule>();
            if (hideRule != null && hideRule.HiddenRenderParts != null)
            {
                _equipPartsHideRule[part].AddRange(hideRule.HiddenRenderParts);
            }

            ApplyHideRule();

            // 파츠 메시 적용
            EquipPartsRenderer?.Equip(part, prefabGO);
        }

        public void UnEquip(CharacterEquipPart part)
        {
            // 장착 룰 복구
            _equipPartsHideRule[part].Clear();
            ApplyHideRule();

            // 파츠 메시 해제
            EquipPartsRenderer?.UnEquip(part);
        }

        private void ApplyHideRule()
        {
            _partsHideSet.Clear();

            // 각 장착 부위의 숨김 룰을 하나의 리스트로 합친다.
            foreach (var pair in _equipPartsHideRule)
            {
                List<CharacterRenderPart> hideParts = pair.Value;
                if (hideParts == null || hideParts.Count == 0)
                    continue;

                foreach (CharacterRenderPart renderPart in hideParts)
                    _partsHideSet.Add(renderPart);
            }

            // 실제 등록된 렌더러만 순회하면서 숨김 여부를 적용한다.
            foreach (var pair in _rendererMap)
            {
                CharacterRenderPart renderPart = pair.Key;
                SkinnedMeshRenderer renderer = pair.Value;

                if (renderer == null)
                    continue;

                bool enabled = !_partsHideSet.Contains(renderPart);
                if (renderer.enabled != enabled)
                    renderer.enabled = enabled;
            }
        }

        public void OnRotate(PointerEventData eventData)
        {
            if (_skeletonRoot == null)
                return;

            float yaw = eventData.delta.x * _rotateSpeed;
            transform.Rotate(0f, -yaw, 0f, Space.Self);
        }
    }
}
