using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Serialization;

namespace PJ.Core.Actor
{
    // 모션 클립 메타데이터 테이블 (데이터 SO)
    // - clipId 를 키로 AnimationClip, 재생될 MotionLayer, 인터럽트 정책을 보관한다.
    // - App 레이어(BehaviorTable) 가 이 테이블을 소유하고, 조회한 Entry 를 View(IMotionPlayer) 로 전달한다.
    // - 같은 clipId 가 어느 캐릭터에서든 동일한 모션을 의미하도록 네트워크 동기화의 진실 단위로 사용한다.
    [CreateAssetMenu(menuName = "CV/Character/MotionClipTable", fileName = "MotionClipTable")]
    public class MotionClipTable : ScriptableObject
    {
        [System.Serializable]
        public struct Entry
        {
            [Tooltip("네트워크 동기화 단위. 0 은 사용하지 않음")]
            public int clipId;

            [FormerlySerializedAs("motionSlot")]
            [FormerlySerializedAs("bodyPart")]
            public MotionLayer motionLayer;

            public AnimationClip clip;

            [Tooltip("재생 시작과 동시에 인터럽트 허용")]
            public bool cancellableFromStart;

            [Range(0f, 1f)]
            [Tooltip("이 normalized time 이후부터 인터럽트 허용 (Animation Event 가 없을 때 fail-safe)")]
            public float cancellableAfter;

            [Range(0f, 0.5f)]
            public float fadeIn;

            [Range(0f, 0.5f)]
            public float fadeOut;
        }

        [SerializeField] private Entry[] _entries;

        private Dictionary<int, Entry> _map;

        public bool TryGet(int clipId, out Entry entry)
        {
            EnsureMap();
            return _map.TryGetValue(clipId, out entry);
        }

        private void EnsureMap()
        {
            if (_map != null)
                return;

            _map = new Dictionary<int, Entry>(_entries != null ? _entries.Length : 0);
            if (_entries == null)
                return;

            foreach (var e in _entries)
            {
                if (e.clipId == 0)
                    continue;

                _map[e.clipId] = e;
            }
        }

        private void OnValidate()
        {
            // 에디터에서 entries 가 바뀌면 다음 조회 시 다시 빌드한다.
            _map = null;
        }
    }
}
