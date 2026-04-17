using System.Collections.Generic;
using System;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    // 카메라별 ScreenBlend 런타임 상태
    public sealed class ScreenBlendRuntimeContext
    {
        // 현재 카메라와 ScreenBlend 를 위한 Snapshot 텍스쳐 핸들
        public RTHandle SnapshotRT;
        public RTHandle InnerSnapshotRT;     // Pass 내부 스냅샷을 사용하는 경우의 텍스쳐
        public float Progress;
        public float ZoomScale = 1f;
        public int Mode;
        // 프레임 체크로 사용되지 않으면 제거를 위한 변수
        internal int LastFrameUsed = -1;

        // 완전 정리 (씬 전환 등)
        internal void Dispose()
        {
            if (SnapshotRT != null)
            {
                SnapshotRT.Release();
                SnapshotRT = null;
            }

            if (InnerSnapshotRT != null)
            {
                InnerSnapshotRT.Release();
                InnerSnapshotRT = null;
            }

            Mode = 0;
            Progress = 0;
            ZoomScale = 1f;
            LastFrameUsed = -1;
        }
    }

    // ScreenBlend 의 런타임 데이터(게임 쪽 데이터)를 위한 Static Class
    public static class ScreenBlendRuntimeContextRegistry
    {
        private struct Entry
        {
            public WeakReference<Camera> CameraRef;
            public ScreenBlendRuntimeContext Context;
        }

        private static readonly Dictionary<int, Entry> _map = new(8);
        private static readonly List<int> _removeBuffer = new(8);
        private static int _lastFrameTick = -1;

        // 플레이 시작 될 때(도메인 초기화시) Unity 에서 호출됨 : static 클래스에 남은 찌꺼기 있으면 초기화 하려고.
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]
        private static void Init()
        {
            ClearAll();

            // app 종료시 정리를 위한 바인딩
            Application.quitting += ClearAll;
        }

        // 컨텍스트 가져오기 (없으면 생성)
        public static ScreenBlendRuntimeContext GetOrCreate(Camera cam)
        {
            if (!cam)
                throw new ArgumentNullException(nameof(cam));

            TickFrame();

            int id = cam.GetInstanceID();

            if (_map.TryGetValue(id, out var entry))
            {
                entry.Context.LastFrameUsed = Time.frameCount;
                return entry.Context;
            }

            var ctx = new ScreenBlendRuntimeContext
            {
                LastFrameUsed = Time.frameCount
            };

            _map[id] = new Entry
            {
                CameraRef = new WeakReference<Camera>(cam),
                Context = ctx
            };

            return ctx;
        }

        // 존재 여부만 확인
        public static bool TryGet(Camera cam, out ScreenBlendRuntimeContext ctx)
        {
            if (cam && _map.TryGetValue(cam.GetInstanceID(), out var entry))
            {
                ctx = entry.Context;
                return true;
            }

            ctx = null;
            return false;
        }

        // Context 제거 : 사용한거 제거
        public static void Release(Camera cam)
        {
            if (!cam) return;

            int id = cam.GetInstanceID();

            if (_map.TryGetValue(id, out var entry))
            {
                entry.Context.Dispose();
                _map.Remove(id);
            }
        }

        // 전체 초기화 (씬 전환 / PlayMode 리셋)
        public static void ClearAll()
        {
            foreach (var e in _map.Values)
                e.Context.Dispose();

            _map.Clear();

            _lastFrameTick = -1;
        }

        // 카메라가 여러개 있어서 TickFrame 이 여러번 불리더라도 frameCount 체크로 한번만 하도록 한다.
        private static void TickFrame()
        {
            int frame = Time.frameCount;

            if (_lastFrameTick == frame)
                return;

            _lastFrameTick = frame;

            if (_map.Count == 0)
                return;

            CleanupDeadCameras();
        }

        // destroyed camera 자동 정리
        private static void CleanupDeadCameras()
        {
            foreach (var kv in _map)
            {
                if (!kv.Value.CameraRef.TryGetTarget(out _))
                    _removeBuffer.Add(kv.Key);
            }

            foreach (var id in _removeBuffer)
            {
                _map[id].Context.Dispose();
                _map.Remove(id);
            }

            _removeBuffer.Clear();
        }
    }
}
