using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.RendererUtils;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    // LayerFilteredRenderFeature
    //
    // 특정 레이어의 오브젝트를 URP(Render Objects 성격) 커스텀 패스로
    // 지정한 RenderPassEvent 시점에 렌더링 하기위한 Renderer Feature 이다.
    // Layer Mask 에 포함된 오브젝트만 이 패스의 대상이 된다.
    // Queue / Sorting / Depth Override 는 이 패스에서 그리는 방식만 제어한다.
    //
    // 사용 규칙
    // - 이 Feature 로만 그리고 싶은 레이어(예: NoPost)는
    //   URP Renderer Asset 의 Opaque Layer Mask / Transparent Layer Mask 에서 제외한다.
    // - 이렇게 하면 기본 DrawOpaqueObjects / DrawTransparentObjects 패스에서는 그리지 않고,
    //   이 Feature 에서만 1번 그리게 되어 중복 렌더를 피할 수 있다.
    //
    // renderPassEvent 주의
    // - RenderPassEvent.AfterRenderingPostProcessing 은
    //   Post Processing 이후이지만 Final Blit, Post AA(Anti-Aliasing), Color Grading 이전 시점이다.
    // - RenderPassEvent.AfterRendering 은 모든 효과 이후의 최종 단계다.
    // - 이 프로젝트에서는 AfterRenderingPostProcessing 에서는 하드웨어 Depth Test 기반 가림이 기대대로 동작했지만,
    //   AfterRendering 에서는 동일한 방식이 기대대로 동작하지 않았다.
    //   그래서 파노라마처럼 "포스트는 최대한 피하고, 앞의 불투명 오브젝트는 유지"가 필요한 경우
    //   현재 프로젝트 기준 권장 시점은 AfterRenderingPostProcessing 이다.
    // - AfterRendering 이 꼭 필요하면, 하드웨어 Depth Test 만 믿지 말고
    //   Shader Graph / Shader 에서 Scene Depth 샘플링으로 직접 해야 한다.
    //
    // 사용 예
    // - NoPost 레이어: 기본 렌더 패스 제외
    // - LayerFilteredRenderFeature: NoPost 만 다시 그리기
    // - renderQueueRange / sortingCriteria 는 프로젝트 목적에 맞게 고정 사용 가능
    //   (예: RenderQueueRange.all + SortingCriteria.CommonTransparent)
    public class LayerFilteredRenderFeature : ScriptableRendererFeature
    {
        [System.Serializable]
        public class Settings
        {
            public string profilerTag = "LayerFilteredRenderPass";
            public RenderPassEvent renderPassEvent = RenderPassEvent.AfterRendering;
            public LayerMask layerMask = 0;

            public bool useOverrideMaterial = false;
            public Material overrideMaterial = null;
            public int overrideMaterialPassIndex = 0;

            public bool overrideDepthState = false;
            public CompareFunction depthCompareFunction = CompareFunction.LessEqual;
            public bool enableDepthWrite = true;
        }

        [SerializeField] private Settings _settings = new();
        private LayerFilteredRenderPass _renderPass;

        public override void Create()
        {
            _renderPass = new LayerFilteredRenderPass(_settings);
            _renderPass.renderPassEvent = _settings.renderPassEvent;
        }

        public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
        {
            if (_settings.layerMask == 0)
                return;

            if (_settings.useOverrideMaterial && _settings.overrideMaterial == null)
                return;

            renderer.EnqueuePass(_renderPass);
        }

        private class LayerFilteredRenderPass : ScriptableRenderPass
        {
            private readonly Settings _settings;
            private readonly ShaderTagId[] _shaderTagIds;

            private class PassData
            {
                public RendererListHandle rendererListHandle;
            }

            public LayerFilteredRenderPass(Settings settings)
            {
                _settings = settings;

                _shaderTagIds = new[]
                {
                    new ShaderTagId("UniversalForward"),
                    new ShaderTagId("UniversalForwardOnly"),
                    new ShaderTagId("SRPDefaultUnlit"),
                };
            }

            public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
            {
                var renderingData = frameData.Get<UniversalRenderingData>();
                var cameraData = frameData.Get<UniversalCameraData>();
                var resourceData = frameData.Get<UniversalResourceData>();

                var renderQueueRange = RenderQueueRange.all;
                var sortingCriteria = SortingCriteria.CommonTransparent;

                var rendererListDesc = new RendererListDesc(_shaderTagIds, renderingData.cullResults, cameraData.camera)
                {
                    sortingCriteria = sortingCriteria,
                    renderQueueRange = renderQueueRange,
                    layerMask = _settings.layerMask,
                };

                if (_settings.useOverrideMaterial && _settings.overrideMaterial != null)
                {
                    rendererListDesc.overrideMaterial = _settings.overrideMaterial;
                    rendererListDesc.overrideMaterialPassIndex = _settings.overrideMaterialPassIndex;
                }

                if (_settings.overrideDepthState)
                {
                    var renderStateBlock = new RenderStateBlock(RenderStateMask.Depth);
                    renderStateBlock.depthState = new DepthState(
                        _settings.enableDepthWrite,
                        _settings.depthCompareFunction);

                    rendererListDesc.stateBlock = renderStateBlock;
                }

                using var builder = renderGraph.AddRasterRenderPass<PassData>(
                    _settings.profilerTag,
                    out var passData);

                passData.rendererListHandle = renderGraph.CreateRendererList(rendererListDesc);

                builder.UseRendererList(passData.rendererListHandle);
                builder.SetRenderAttachment(resourceData.activeColorTexture, 0);
                builder.SetRenderAttachmentDepth(resourceData.activeDepthTexture, AccessFlags.Write);
                builder.AllowPassCulling(false);

                builder.SetRenderFunc(static (PassData data, RasterGraphContext context) =>
                {
                    context.cmd.DrawRendererList(data.rendererListHandle);
                });
            }
        }
    }
}
