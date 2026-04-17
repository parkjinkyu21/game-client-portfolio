using System;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace PJ.Core.Rendering
{
    [VolumeComponentMenu("CV/ScreenBlend")]
    [VolumeRequiresRendererFeatures(typeof(ScreenBlendFeature))]
    [SupportedOnRenderPipeline(typeof(UniversalRenderPipelineAsset))]
    // URP Volume 시스템에서 ScreenBlend 효과의 활성/비활성을 제어하는 컴포넌트.
    public sealed class ScreenBlendVolume : VolumeComponent, IPostProcessComponent
    {
        // 인스펙터에서 ScreenBlend 사용 여부.
        [SerializeField]
        private BoolParameter _active = new(true);
        public bool IsActive() => _active.value;
    }
}
