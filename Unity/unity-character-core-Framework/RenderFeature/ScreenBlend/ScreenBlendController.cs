using DG.Tweening;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    // 화면 전환 효과의 게임 로직 측 컨트롤러.
    // - Play 호출 시 현재 카메라 화면을 스냅샷으로 캡처하고,
    //   DOTween 으로 Progress 를 0→1 로 애니메이션하여 ZoomOverlay 전환 효과를 재생한다.
    // - ScreenBlendRuntimeContextRegistry 에 카메라별 상태를 등록하고,
    //   ScreenBlendPass(RenderGraph) 가 매 프레임 해당 상태를 읽어 렌더링한다.
    public class ScreenBlendController : MonoBehaviour
    {
        [SerializeField] private float _playTime = 0.2f;
        [SerializeField] private float _zoomScale = 1.5f;

        // TargetCamera 는 런타임 및 인스펙터에서 등록해준다.
        [SerializeField] private Camera _targetCamera;
        public Camera TargetCamera { get => _targetCamera; set => _targetCamera = value; }

        private Tween _tween;
        private float _currProgress;

        public bool IsPlaying() => _tween != null;

        private void OnDestroy()
        {
            ScreenBlendRuntimeContextRegistry.Release(TargetCamera);
        }

        public void Play()
        {
            Stop();
            RegisterScreenBlendCtx();

            if (!ScreenBlendRuntimeContextRegistry.TryGet(TargetCamera, out ScreenBlendRuntimeContext runtimeCtx))
            {
                Debug.LogWarning("[ScreenBlendController] Play 실패: RuntimeContext 를 가져올 수 없습니다.");
                return;
            }

            _currProgress = 0f;
            runtimeCtx.Progress = _currProgress;

            _tween = DOTween.To(
                () => _currProgress,
                value =>
                {
                    // 클로저 캡처 대신 매번 Registry 에서 조회하여 최신 Context 에 반영한다.
                    if (ScreenBlendRuntimeContextRegistry.TryGet(TargetCamera, out ScreenBlendRuntimeContext ctx))
                    {
                        ctx.Progress = value;
                    }
                },
                1f,
                _playTime)
                .SetEase(Ease.OutQuad)
                .OnKill(() =>
                {
                    _tween = null;
                    Stop();
                });
        }

        public void Stop()
        {
            ScreenBlendRuntimeContextRegistry.Release(TargetCamera);
            _tween?.Kill();
            _tween = null;
        }

        // 카메라 스냅샷을 캡처하고 RuntimeContext 에 등록한다.
        private void RegisterScreenBlendCtx()
        {
            if (TargetCamera == null)
                return;

            var ctx = ScreenBlendRuntimeContextRegistry.GetOrCreate(TargetCamera);

            var rt = RuntimeSnapshotUtil.CaptureSnapshot(TargetCamera);
            ctx.SnapshotRT?.Release();

            ctx.SnapshotRT = RTHandles.Alloc(rt);

            ctx.Progress = 0f;
            ctx.ZoomScale = _zoomScale;
        }
    }
}
