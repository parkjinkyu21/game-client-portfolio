using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    // ScreenBlend 효과를 URP 렌더러에 등록하는 ScriptableRendererFeature.
    // - 셰이더로부터 머터리얼을 생성하고 ScreenBlendPass 를 만든다.
    // - 매 프레임 카메라에 RuntimeContext 가 있을 때만 패스를 큐에 추가한다.
    public class ScreenBlendFeature : ScriptableRendererFeature
    {
        [SerializeField] private Shader _shader;

        private Material _mat;
        private ScreenBlendPass _pass;

        public override void Create()
        {
            if (!_shader) return;

            _mat = CoreUtils.CreateEngineMaterial(_shader);

            _pass = new ScreenBlendPass("ScreenBlend", _mat)
            {
                renderPassEvent = RenderPassEvent.AfterRenderingPostProcessing
            };
        }

        public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData data)
        {
            if (_mat == null || _pass == null) return;

            // RuntimeContext 에 있으면 패스에 추가한다.
            if (ScreenBlendRuntimeContextRegistry.TryGet(data.cameraData.camera, out ScreenBlendRuntimeContext runtimeCtx))
            {
                renderer.EnqueuePass(_pass);
            }
        }

        protected override void Dispose(bool disposing)
        {
            CoreUtils.Destroy(_mat);
            _mat = null;
        }
    }
}
