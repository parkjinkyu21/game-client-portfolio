using System.Collections.Generic;
using System.Threading;
using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.AddressableAssets;
using UnityEngine.ResourceManagement.AsyncOperations;

namespace PJ.Core.Actor
{
    // 캐릭터의 생성, 제거, 조회와 장착/해제를 관리하는 서비스
    // - CharacterActor를 ID 기준으로 관리한다.
    // - 캐릭터 Spawn / Despawn / Get을 제공한다.
    // - 장비 Equip / UnEquip 요청을 해당 CharacterActor에 전달한다.
    // - Addressables 를 직접 사용하여 프리팹 로드/해제를 수행한다.
    public class CharacterService : MonoBehaviour
    {
        private readonly Dictionary<string, CharacterActor> _actors = new();

        // Spawn 으로 생성된 오브젝트의 Addressable 핸들. Despawn 시 해제한다.
        private readonly Dictionary<string, AsyncOperationHandle<GameObject>> _spawnHandles = new();

        // 진행 중인 Spawn 의 버전 토큰. id 단위로 관리한다.
        // - 같은 id 로 Spawn/Despawn/Clear 가 들어오면 이 값을 ++ 시켜 in-flight 를 무효화한다.
        private readonly Dictionary<string, int> _spawnVersions = new();

        // id 단위로 Spawn 취소용 CancellationTokenSource 를 관리한다.
        private readonly Dictionary<string, CancellationTokenSource> _spawnCts = new();

        private bool _destroyed;
        private CancellationToken _destroyToken;

        private void Awake()
        {
            _destroyToken = this.GetCancellationTokenOnDestroy();
        }

        private void OnDestroy()
        {
            _destroyed = true;
            Clear();
        }

        // 캐릭터 생성 (UniTask 비동기)
        // 성공 시 CharacterActor 를 반환하고, 실패 시 null 을 반환한다.
        public async UniTask<CharacterActor> SpawnAsync(string id, string assetKey, Vector3 position, Quaternion rotation)
        {
            if (_actors.ContainsKey(id))
            {
                Debug.LogWarning($"[CharacterService] Character already exists : {id}");
                return null;
            }

            // 새 요청 버전 발급. 이전 in-flight 가 있으면 자동으로 무효 처리된다.
            int version = GetNextSpawnVersion(id);
            CancelSpawn(id);

            var cts = CancellationTokenSource.CreateLinkedTokenSource(_destroyToken);
            _spawnCts[id] = cts;

            AsyncOperationHandle<GameObject> handle = Addressables.LoadAssetAsync<GameObject>(assetKey);

            try
            {
                GameObject prefab = await handle.ToUniTask(cancellationToken: cts.Token);

                // 서비스가 파괴되었거나, 같은 id 로 더 새 요청이 들어왔거나, Despawn 으로 취소된 경우
                if (_destroyed || !IsLatestSpawnVersion(id, version))
                {
                    if (handle.IsValid())
                        Addressables.Release(handle);
                    return null;
                }

                _spawnCts.Remove(id);

                if (handle.Status != AsyncOperationStatus.Succeeded || prefab == null)
                {
                    Debug.LogWarning($"[CharacterService] Prefab not found from Addressable : {assetKey}");
                    if (handle.IsValid())
                        Addressables.Release(handle);
                    _spawnVersions.Remove(id);
                    return null;
                }

                // 프리팹을 인스턴스화한다.
                GameObject go = Object.Instantiate(prefab, position, rotation, transform);

                CharacterActor actor = go.GetComponent<CharacterActor>();
                if (actor == null)
                {
                    Debug.LogWarning("[CharacterService] CharacterActor missing.");
                    Object.Destroy(go);
                    if (handle.IsValid())
                        Addressables.Release(handle);
                    _spawnVersions.Remove(id);
                    return null;
                }

                _spawnVersions.Remove(id);
                _spawnHandles[id] = handle;
                actor.Initialize(id);
                _actors.Add(id, actor);

                return actor;
            }
            catch (System.OperationCanceledException)
            {
                if (handle.IsValid())
                    Addressables.Release(handle);
                return null;
            }
        }

        // 캐릭터 제거
        public void Despawn(string id)
        {
            // 로딩 중이면 취소
            CancelSpawn(id);
            _spawnVersions.Remove(id);

            if (!_actors.TryGetValue(id, out var actor))
                return;

            _actors.Remove(id);

            if (actor != null)
                Object.Destroy(actor.gameObject);

            // Addressable 핸들 해제
            if (_spawnHandles.TryGetValue(id, out var handle))
            {
                if (handle.IsValid())
                    Addressables.Release(handle);
                _spawnHandles.Remove(id);
            }
        }

        private void CancelSpawn(string id)
        {
            if (_spawnCts.TryGetValue(id, out var cts))
            {
                cts.Cancel();
                cts.Dispose();
                _spawnCts.Remove(id);
            }
        }

        private int GetNextSpawnVersion(string id)
        {
            if (!_spawnVersions.TryGetValue(id, out int version))
                version = 0;

            version++;
            _spawnVersions[id] = version;
            return version;
        }

        private bool IsLatestSpawnVersion(string id, int version)
        {
            return _spawnVersions.TryGetValue(id, out int current) && current == version;
        }

        // 캐릭터 조회
        public CharacterActor Get(string id)
        {
            _actors.TryGetValue(id, out var actor);
            return actor;
        }

        // 장착
        public void Equip(string id, CharacterEquipPart part, string prefabPath)
        {
            if (!_actors.TryGetValue(id, out var actor))
            {
                Debug.LogWarning($"[CharacterService] Character not found : {id}");
                return;
            }

            actor.Equip(part, prefabPath);
        }

        // 해제
        public void UnEquip(string id, CharacterEquipPart part)
        {
            if (!_actors.TryGetValue(id, out var actor))
            {
                Debug.LogWarning($"[CharacterService] Character not found : {id}");
                return;
            }

            actor.UnEquip(part);
        }

        // 모든 캐릭터 제거
        public void Clear()
        {
            // 진행 중인 Spawn 을 모두 취소한다.
            foreach (var kv in _spawnCts)
            {
                kv.Value.Cancel();
                kv.Value.Dispose();
            }
            _spawnCts.Clear();
            _spawnVersions.Clear();

            foreach (var actor in _actors.Values)
            {
                if (actor != null)
                    Object.Destroy(actor.gameObject);
            }
            _actors.Clear();

            // Addressable 핸들을 모두 해제한다.
            foreach (var kv in _spawnHandles)
            {
                if (kv.Value.IsValid())
                    Addressables.Release(kv.Value);
            }
            _spawnHandles.Clear();
        }
    }
}
