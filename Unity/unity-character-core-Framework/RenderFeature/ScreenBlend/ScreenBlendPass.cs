using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    // RenderGraph 기반 화면 전환 렌더 패스.
    // - 캡처된 스냅샷 텍스처를 Zoom + Alpha 조절하여 카메라 출력 위에 오버레이한다.
    // - 스냅샷이 없으면 패스 내부에서 현재 프레임을 캡처하여 사용한다.
    // - RuntimeContext 의 Progress, ZoomScale, Mode 값을 셰이더에 전달한다.
    public class ScreenBlendPass : ScriptableRenderPass
    {
        private class PassData
        {
            public TextureHandle source;
            public Material material;
            public TextureHandle snapshotHandle;
            public int mode;
            public float progress;
            public float zoomScale;
        }

        private readonly Material _material;
        private readonly string _passName;

        private static readonly int kSnapshotTex = Shader.PropertyToID("_SnapshotTex");
        private static readonly int kMode = Shader.PropertyToID("_Mode");
        private static readonly int kProgress = Shader.PropertyToID("_Progress");
        private static readonly int kZoomScale = Shader.PropertyToID("_ZoomScale");

        public ScreenBlendPass(string passName, Material material)
        {
            _passName = passName;
            _material = material;

            profilingSampler = new ProfilingSampler(_passName);
            requiresIntermediateTexture = true;
        }

        // RenderGraph 메인 함수
        public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
        {
            if (_material == null) return;

            ZoomOverlayRenderGraph(renderGraph, frameData);
        }

        // 스냅샷을 Zoom + Alpha 를 조절하여 카메라 데이터에 오버레이로 덮는 방식
        private void ZoomOverlayRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
        {
            var camData = frameData.Get<UniversalCameraData>();
            var camDesc = camData.cameraTargetDescriptor;
            camDesc.depthBufferBits = 0;

            ScreenBlendRuntimeContext runtimeCtx = ScreenBlendRuntimeContextRegistry.GetOrCreate(camData.camera);
            RTHandle snapshotRT = runtimeCtx.SnapshotRT;

            // context 에 snapshotHandle 이 없으면 패스 내부에서 스냅샷을 만든다.
            bool needSnapshot = false;
            if (snapshotRT == null)
            {
                RenderingUtils.ReAllocateHandleIfNeeded(ref runtimeCtx.InnerSnapshotRT, camDesc, name: "_ScreenBlendSnapshotTex");
                snapshotRT = runtimeCtx.InnerSnapshotRT;
                needSnapshot = true;
            }

            TextureHandle snapshotHandle = renderGraph.ImportTexture(snapshotRT);

            var resData = frameData.Get<UniversalResourceData>();
            var frameTexture = resData.activeColorTexture;

            if (needSnapshot)
            {
                // 패스 내부에서 스냅샷을 만든다.
                using (var builder = renderGraph.AddRasterRenderPass<PassData>("InnerSnapshot", out var pass))
                {
                    pass.source = frameTexture;

                    builder.UseTexture(frameTexture, AccessFlags.Read);
                    builder.SetRenderAttachment(snapshotHandle, 0);

                    builder.SetRenderFunc<PassData>((PassData data, RasterGraphContext ctx) =>
                    {
                        Blitter.BlitTexture(ctx.cmd, data.source, new Vector4(1, 1, 0, 0), 0f, false);
                    });
                }
            }

            // RenderGraph 원칙에 따라 모든 값을 PassData 에 복사한다.
            using (var builder = renderGraph.AddRasterRenderPass<PassData>("ZoomOverlay", out var pass))
            {
                pass.material = _material;
                pass.snapshotHandle = snapshotHandle;
                pass.mode = runtimeCtx.Mode;
                pass.progress = runtimeCtx.Progress;
                pass.zoomScale = runtimeCtx.ZoomScale;

                builder.UseTexture(snapshotHandle, AccessFlags.Read);
                builder.SetRenderAttachment(frameTexture, 0);

                builder.SetRenderFunc((PassData data, RasterGraphContext ctx) =>
                {
                    data.material.SetTexture(kSnapshotTex, data.snapshotHandle);
                    data.material.SetFloat(kMode, data.mode);
                    data.material.SetFloat(kProgress, data.progress);
                    data.material.SetFloat(kZoomScale, data.zoomScale);

                    CoreUtils.DrawFullScreen(ctx.cmd, data.material);
                });
            }
        }

        public override void OnCameraCleanup(CommandBuffer cmd)
        {
            // 스냅샷 RT 는 다음 Play 까지 유지해야 하므로 여기서 해제하지 않는다.
        }
    }
}
