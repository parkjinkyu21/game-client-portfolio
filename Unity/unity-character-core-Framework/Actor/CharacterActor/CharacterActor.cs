using System.Collections.Generic;
using System.Threading;
using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

namespace PJ.Core.Actor
{
    // 캐릭터 본체와 장착 파츠 로드를 관리하는 액터
    // - CharacterView와 CharacterAnimator를 보관하고 사용한다.
    // - 장착 파츠의 Addressables 로드, 취소, 해제 수명 주기를 직접 관리한다.
    // - 파츠 에셋의 로드 요청 소유권과 처리 책임은 CharacterActor가 가진다.
    // - 같은 부위에 연속 Equip 요청이 들어오면 최신 요청만 유효하게 처리한다.
    public class CharacterActor : Actor
    {
        [System.Serializable]
        public struct DefaultEquipPart
        {
            public CharacterEquipPart part;
            public string prefabName;
        }

        [SerializeField] private DefaultEquipPart[] _defaultEquipParts;

        public CharacterView View { get; private set; }
        public CharacterAnimator Animator { get; private set; }

        // 현재 실제로 장착 중인 핸들
        private readonly Dictionary<CharacterEquipPart, AsyncOperationHandle<GameObject>> _equippedHandles = new();

        // 동일 부위 연속 Equip 요청 구분용 버전 토큰
        private readonly Dictionary<CharacterEquipPart, int> _loadVersions = new();

        // 부위별 로드 취소용 CancellationTokenSource
        private readonly Dictionary<CharacterEquipPart, CancellationTokenSource> _loadCts = new();

        private int _defaultEquipRemaining;
        private CancellationToken _destroyToken;

        private void Awake()
        {
            if (View == null)
                View = GetComponentInChildren<CharacterView>();

            if (Animator == null)
                Animator = GetComponentInChildren<CharacterAnimator>();

            _destroyToken = this.GetCancellationTokenOnDestroy();
        }

        private void Start()
        {
            if (_defaultEquipParts != null && _defaultEquipParts.Length > 0)
            {
                _defaultEquipRemaining = _defaultEquipParts.Length;
                SetVisible(false);

                foreach (var def in _defaultEquipParts)
                {
                    Equip(def.part, def.prefabName);
                }
            }
        }

        public void SetDefaultEquipParts(DefaultEquipPart[] defaults)
        {
            _defaultEquipParts = defaults ?? System.Array.Empty<DefaultEquipPart>();
        }

        public void SetVisible(bool visible)
        {
            if (View != null)
                View.SetVisible(visible);
        }

        private void OnDestroy()
        {
            Cleanup();
        }

        // Addressables 를 통해 장착 파츠를 비동기 로드한다.
        // 같은 부위에 이미 로드 중인 요청이 있으면 취소한 뒤 새 요청을 시작한다.
        public virtual void Equip(CharacterEquipPart part, string prefabName)
        {
            OnEquip(part, prefabName);

            // 기존 로딩 중인 요청이 있으면 취소
            CancelLoading(part);

            int version = GetNextLoadVersion(part);

            var cts = CancellationTokenSource.CreateLinkedTokenSource(_destroyToken);
            _loadCts[part] = cts;

            EquipAsync(part, prefabName, version, cts.Token).Forget();
        }

        private async UniTaskVoid EquipAsync(CharacterEquipPart part, string prefabName, int version, CancellationToken token)
        {
            AsyncOperationHandle<GameObject> handle = Addressables.LoadAssetAsync<GameObject>(prefabName);

            try
            {
                GameObject result = await handle.ToUniTask(cancellationToken: token);

                // 더 최신 Equip 요청이 들어온 상태면 무시
                if (!IsLatestLoadVersion(part, version))
                {
                    if (handle.IsValid())
                        Addressables.Release(handle);
                    return;
                }

                _loadCts.Remove(part);

                if (handle.Status != AsyncOperationStatus.Succeeded || result == null)
                {
                    // 실패해도 이전 장비는 유지
                    Debug.LogError($"[CharacterActor] Equip Load Failed: {prefabName}");
                    if (handle.IsValid())
                        Addressables.Release(handle);

                    OnEquipLoaded(part, prefabName, null);
                    return;
                }

                // 새 로드 성공 후에만 기존 장착 해제
                UnEquipCurrent(part);

                View?.Equip(part, result);

                // 새 핸들을 현재 장착 핸들로 승격
                _equippedHandles[part] = handle;

                OnEquipLoaded(part, prefabName, result);
            }
            catch (System.OperationCanceledException)
            {
                if (handle.IsValid())
                    Addressables.Release(handle);
            }
        }

        protected virtual void OnEquip(CharacterEquipPart part, string prefabName)
        {
        }

        protected virtual void OnEquipLoaded(CharacterEquipPart part, string prefabName, GameObject ob)
        {
            // ob가 null 이면 로드 실패
            // 실패해도 카운터를 줄여 기본 복장 한 파츠가 깨져도 캐릭터가 영원히 invisible 상태로 남지 않도록 한다.
            if (_defaultEquipRemaining > 0)
            {
                _defaultEquipRemaining--;
                // 로드 다 되면 화면에 보이도록 한다.
                if (_defaultEquipRemaining == 0)
                    SetVisible(true);
            }
        }

        public virtual void UnEquip(CharacterEquipPart part)
        {
            OnUnEquip(part);

            // 아직 로딩 중인 요청 취소
            CancelLoading(part);

            // 현재 장착 중인 것 제거
            UnEquipCurrent(part);
        }

        protected virtual void OnUnEquip(CharacterEquipPart part)
        {
        }

        private void UnEquipCurrent(CharacterEquipPart part)
        {
            View?.UnEquip(part);

            if (_equippedHandles.TryGetValue(part, out var handle))
            {
                if (handle.IsValid())
                    Addressables.Release(handle);

                _equippedHandles.Remove(part);
            }
        }

        private void CancelLoading(CharacterEquipPart part)
        {
            if (_loadCts.TryGetValue(part, out var cts))
            {
                cts.Cancel();
                cts.Dispose();
                _loadCts.Remove(part);
            }
        }

        private int GetNextLoadVersion(CharacterEquipPart part)
        {
            if (!_loadVersions.TryGetValue(part, out int version))
                version = 0;

            version++;
            _loadVersions[part] = version;
            return version;
        }

        private bool IsLatestLoadVersion(CharacterEquipPart part, int version)
        {
            return _loadVersions.TryGetValue(part, out int currentVersion) && currentVersion == version;
        }

        protected virtual void Cleanup()
        {
            // 로딩 중인 요청을 모두 취소한다.
            foreach (var kv in _loadCts)
            {
                kv.Value.Cancel();
                kv.Value.Dispose();
            }
            _loadCts.Clear();

            // 장착 중인 핸들을 모두 해제한다.
            foreach (var kv in _equippedHandles)
            {
                var handle = kv.Value;
                if (handle.IsValid())
                    Addressables.Release(handle);
            }

            _equippedHandles.Clear();
            _loadVersions.Clear();
        }
    }
}
