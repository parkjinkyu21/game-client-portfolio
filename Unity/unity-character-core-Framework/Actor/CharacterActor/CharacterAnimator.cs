using System;
using System.Collections.Generic;
using System.Threading;
using Cysharp.Threading.Tasks;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.Serialization;
#if UNITY_EDITOR
using UnityEditor.Animations;
#endif

namespace PJ.Core.Actor
{
    // 캐릭터 Animator 제어 + 모션 오버레이 재생을 담당하는 View 컴포넌트
    // - Base 레이어 (index 0) 는 로코모션 State Machine 이 항상 돌아가는 레이어. 이 컴포넌트는 건드리지 않음.
    // - 로코모션 State Machine 은 외부에서 파라미터를 설정하도록 한다.
    // - 모션 오버레이 = clipId 단위로 재생되는 Upper/Lower/Full 3 개의 슬롯 (Base 위에 얹힘).
    //   · Upper/Lower : Avatar Mask 로 상/하체 영역 분리, 서로 공존 가능
    //   · Full         : 마스크 없음, 전신을 덮음 (Upper/Lower 와 상호 배타)
    // - 같은 슬롯에서 클립을 연속 재생하면 자세 보간이 일어나지 않아 한 프레임 팝(pop) 이 보일 수 있다.
    //   현 구조에서는 감수하고, 향후 Animancer 등 유료 에셋으로 대체할 예정.
    // - 모션 재생: 호출자가 전달한 MotionClipTable.Entry 의 클립을 해당 오버레이 슬롯 State 에 런타임 swap
    // - View 는 MotionClipTable 을 소유하지 않는다. 어떤 엔트리를 재생할지는 App 레이어가 결정한다.
    // - Animation Clip Event 를 외부 로직으로 전달한다 (OnAnimationEvent / OnAnimationEventRaw)
    public class CharacterAnimator : MonoBehaviour, IMotionPlayer
    {
        // 슬롯 신호 컨벤션 — 클립 위 Animation Event 의 stringParameter 로 마킹한다
        // - "Motion_Cancellable_{Upper|Lower|Full}"
        // - "Motion_End_{Upper|Lower|Full}"
        private const string EVT_CANCELLABLE_UPPER = "Motion_Cancellable_Upper";
        private const string EVT_CANCELLABLE_LOWER = "Motion_Cancellable_Lower";
        private const string EVT_CANCELLABLE_FULL = "Motion_Cancellable_Full";
        private const string EVT_END_UPPER = "Motion_End_Upper";
        private const string EVT_END_LOWER = "Motion_End_Lower";
        private const string EVT_END_FULL = "Motion_End_Full";

        private static readonly int IdleHash = Animator.StringToHash("Idle");

        // 슬롯 enum 순회용 정적 캐시 (박싱 회피)
        private static readonly MotionLayer[] MotionLayerValues =
            (MotionLayer[])Enum.GetValues(typeof(MotionLayer));

        [SerializeField] private Animator _animator;

        [Header("Motion Overlay Layers")]
        [SerializeField] private SlotConfig _fullSlotConfig = new SlotConfig { stateName = "Full", layer = 1 };
        [SerializeField] private SlotConfig _upperSlotConfig = new SlotConfig { stateName = "Upper", layer = 2 };
        [SerializeField] private SlotConfig _lowerSlotConfig = new SlotConfig { stateName = "Lower", layer = 3 };

        public UnityAction<string> OnAnimationEvent;
        public UnityAction<AnimationEvent> OnAnimationEventRaw;

        // IMotionPlayer 이벤트
        public event Action<int> OnClipStarted;
        public event Action<int> OnClipCancellable;
        public event Action<int> OnClipEnded;

        private AnimatorOverrideController _overrideCtrl;

        // _slots[(int)MotionLayer] 로 인덱싱한다. Awake 에서 채운다.
        private SlotRuntime[] _slots;

        // 슬롯 신호 이름 → (슬롯 인덱스, 종료신호 여부) 매핑. 매 이벤트 호출마다 switch 가 아닌 dict lookup.
        private static readonly Dictionary<string, SlotSignal> SignalMap = new()
        {
            { EVT_CANCELLABLE_UPPER, new SlotSignal(MotionLayer.Upper, false) },
            { EVT_CANCELLABLE_LOWER, new SlotSignal(MotionLayer.Lower, false) },
            { EVT_CANCELLABLE_FULL,  new SlotSignal(MotionLayer.Full,  false) },
            { EVT_END_UPPER, new SlotSignal(MotionLayer.Upper, true) },
            { EVT_END_LOWER, new SlotSignal(MotionLayer.Lower, true) },
            { EVT_END_FULL,  new SlotSignal(MotionLayer.Full,  true) },
        };

        private bool _slotsEnabled;
        private CancellationToken _destroyToken;

        // ============================================================
        // 직렬화 / 런타임 보조 타입
        // ============================================================

        [Serializable]
        private class SlotConfig
        {
            [Tooltip("AnimatorController 안에서 슬롯 역할을 하는 State 의 이름으로 LayerName.StateName 으로 설정한다.")]
            [FormerlySerializedAs("stateName")]
            public string stateName;

            [Tooltip("해당 State 가 속한 레이어 인덱스")]
            public int layer;

            // 에디터에서 OnValidate 가 AnimatorController 의 State.motion 으로부터 자동 채워준다.
            [Tooltip("이 State 가 참조하고 있는 placeholder AnimationClip. 런타임에 이 클립이 swap 된다 (에디터에서 자동 동기화됨)")]
            [FormerlySerializedAs("placeholderClip")]
            public AnimationClip placeholderClip;
        }

        private class SlotRuntime
        {
            public SlotConfig config;
            public int currentClipId;
            public MotionClipTable.Entry currentEntry;
            public float startTime;
            public bool cancellable;
            public bool occupied;
            public int stateHash;
            // 비동기 작업 취소용 — 새 fade 가 시작되거나 슬롯이 교체될 때 이전 작업을 취소한다
            public CancellationTokenSource fadeCts;
            public CancellationTokenSource lifecycleCts;
        }

        private readonly struct SlotSignal
        {
            public readonly MotionLayer layer;
            public readonly bool isEnd; // true=종료 신호, false=cancellable 신호

            public SlotSignal(MotionLayer layer, bool isEnd)
            {
                this.layer = layer;
                this.isEnd = isEnd;
            }
        }

        // ============================================================
        // 수명 주기
        // ============================================================

        private void Awake()
        {
            if (_animator == null)
                _animator = GetComponent<Animator>();

            _destroyToken = this.GetCancellationTokenOnDestroy();

            // _slots[(int)MotionLayer] 로 접근할 수 있도록 enum 인덱스 순서대로 채운다.
            _slots = new SlotRuntime[MotionLayerValues.Length];
            _slots[(int)MotionLayer.Upper] = CreateSlot(_upperSlotConfig);
            _slots[(int)MotionLayer.Lower] = CreateSlot(_lowerSlotConfig);
            _slots[(int)MotionLayer.Full]  = CreateSlot(_fullSlotConfig);

            TryInstallOverrideController();

            PlayAnimation(IdleHash);
        }

        private SlotRuntime CreateSlot(SlotConfig config)
        {
            var slot = new SlotRuntime { config = config };
            if (config != null && !string.IsNullOrEmpty(config.stateName))
                slot.stateHash = Animator.StringToHash(config.stateName);
            return slot;
        }

        private void OnDestroy()
        {
            if (_slots == null)
                return;

            for (int i = 0; i < _slots.Length; i++)
            {
                CancelFade(_slots[i]);
                CancelLifecycle(_slots[i]);
            }
        }

        private void TryInstallOverrideController()
        {
            _slotsEnabled = false;

            if (_animator == null || _animator.runtimeAnimatorController == null)
                return;

            bool anyPlaceholder = false;
            for (int i = 0; i < _slots.Length; i++)
            {
                if (_slots[i]?.config?.placeholderClip != null)
                {
                    anyPlaceholder = true;
                    break;
                }
            }

            if (!anyPlaceholder)
                return;

            _overrideCtrl = new AnimatorOverrideController(_animator.runtimeAnimatorController);
            _animator.runtimeAnimatorController = _overrideCtrl;
            _slotsEnabled = true;
        }

        // ============================================================
        // 기본 Animator 래핑
        // ============================================================

        public Animator GetAnimator()
        {
            return _animator;
        }

        public void PlayAnimation(int hash, int layer = 0, float normalizedTime = 0f)
        {
            if (_animator == null)
                return;

            _animator.Play(hash, layer, normalizedTime);
        }

        public void CrossFade(int hash, float time = 0.15f, int layer = 0, float normalizedTime = 0f)
        {
            if (_animator == null)
                return;

            _animator.CrossFade(hash, time, layer, normalizedTime);
        }

        public void SetBool(int hash, bool value)
        {
            if (_animator == null)
                return;

            _animator.SetBool(hash, value);
        }

        public void SetFloat(int hash, float value)
        {
            if (_animator == null)
                return;

            _animator.SetFloat(hash, value);
        }

        public void SetTrigger(int hash)
        {
            if (_animator == null)
                return;

            _animator.SetTrigger(hash);
        }

        public void ResetTrigger(int hash)
        {
            if (_animator == null)
                return;

            _animator.ResetTrigger(hash);
        }

        // ============================================================
        // Animation Clip Event 라우팅 (기존 + 슬롯 신호 변환)
        // ============================================================

        // Animation Clip Event에서 문자열 이벤트 이름을 전달할 때 사용한다.
        public void AnimEvent_String(string eventName)
        {
#if UNITY_EDITOR || DEVELOPMENT_BUILD
            Debug.Log($"[CharacterAnimator] eventName : {eventName}");
#endif
            OnAnimationEvent?.Invoke(eventName);

            // 슬롯 신호로 변환 — dict lookup 로 6-way switch 대체
            if (eventName != null && SignalMap.TryGetValue(eventName, out var signal))
            {
                var slot = GetSlot(signal.layer);
                if (signal.isEnd)
                    EndSlot(slot, immediate: false);
                else
                    MarkCancellable(slot);
            }
        }

        // Animation Clip Event에서 AnimationEvent 전체 정보를 전달할 때 사용한다.
        public void AnimEvent_Raw(AnimationEvent evt)
        {
            if (evt == null)
                return;

#if UNITY_EDITOR || DEVELOPMENT_BUILD
            Debug.Log(
                $"[CharacterAnimator] eventName : {evt.stringParameter}, " +
                $"int : {evt.intParameter}, " +
                $"float : {evt.floatParameter}, " +
                $"object : {evt.objectReferenceParameter}");
#endif

            OnAnimationEventRaw?.Invoke(evt);
        }

        // ============================================================
        // IMotionPlayer 구현 — 모션 레이어 재생
        // ============================================================

        public bool CanPlay(in MotionClipTable.Entry entry)
        {
            if (!_slotsEnabled)
                return false;

            if (entry.clipId == 0 || entry.clip == null)
                return false;

            var target = GetSlot(entry.motionLayer);
            if (target == null)
                return false;

            // placeholderClip 이 없으면 swap 대상 자체가 없으므로 재생 불가
            if (target.config == null || target.config.placeholderClip == null)
                return false;

            if (!IsSlotInterruptible(target))
                return false;

            // 상호 배제 검사 — Full 은 Upper/Lower 와 같은 몸을 놓고 싸운다
            switch (entry.motionLayer)
            {
                case MotionLayer.Full:
                    if (!IsSlotInterruptible(GetSlot(MotionLayer.Upper)))
                        return false;
                    if (!IsSlotInterruptible(GetSlot(MotionLayer.Lower)))
                        return false;
                    break;
                case MotionLayer.Upper:
                case MotionLayer.Lower:
                    if (!IsSlotInterruptible(GetSlot(MotionLayer.Full)))
                        return false;
                    break;
            }

            return true;
        }

        public bool Play(in MotionClipTable.Entry entry)
        {
            if (!CanPlay(in entry))
                return false;

            var target = GetSlot(entry.motionLayer);

            // 상호 배제 적용 — 새 클립의 fadeIn 에 맞춰 상대 슬롯을 페이드 아웃
            switch (entry.motionLayer)
            {
                case MotionLayer.Full:
                    StopSlot(GetSlot(MotionLayer.Upper), entry.fadeIn);
                    StopSlot(GetSlot(MotionLayer.Lower), entry.fadeIn);
                    break;
                case MotionLayer.Upper:
                case MotionLayer.Lower:
                    StopSlot(GetSlot(MotionLayer.Full), entry.fadeIn);
                    break;
            }

            if (!PlayOnSlot(target, entry))
                return false;

            OnClipStarted?.Invoke(entry.clipId);
            return true;
        }

        public void StopAll(float fadeOut = 0.15f)
        {
            if (_slots == null)
                return;

            for (int i = 0; i < _slots.Length; i++)
                StopSlot(_slots[i], fadeOut);
        }

        // ============================================================
        // 슬롯 내부 로직
        // ============================================================

        private bool PlayOnSlot(SlotRuntime slot, MotionClipTable.Entry entry)
        {
            if (slot == null)
                return false;

            // 이전 lifecycle 작업 취소
            CancelLifecycle(slot);

            if (_overrideCtrl == null || slot.config?.placeholderClip == null)
                return false;

            // OverrideController 의 placeholder 를 실제 클립으로 swap
            _overrideCtrl[slot.config.placeholderClip] = entry.clip;

            slot.currentEntry = entry;
            slot.currentClipId = entry.clipId;
            slot.startTime = Time.time;
            slot.cancellable = entry.cancellableFromStart;
            slot.occupied = true;

            _animator.Play(slot.stateHash, slot.config.layer, 0f);

            StartFade(slot, target: 1f, duration: entry.fadeIn);

            // 새로운 슬롯 시작
            StartLifecycle(slot);

            return true;
        }

        private void StopSlot(SlotRuntime slot, float fadeOut)
        {
            if (slot == null || !slot.occupied)
                return;

            CancelLifecycle(slot);

            int endedClipId = slot.currentClipId;

            StartFade(slot, target: 0f, duration: fadeOut);

            // 오버라이드를 원래 placeholder 로 되돌린다.
            // weight 0 레이어에서 무거운 모션 클립이 계속 샘플링/이벤트 발행되는 것을 방지한다.
            if (_overrideCtrl != null && slot.config?.placeholderClip != null)
                _overrideCtrl[slot.config.placeholderClip] = slot.config.placeholderClip;

            slot.occupied = false;
            slot.currentClipId = 0;
            slot.cancellable = false;
            slot.currentEntry = default;

            if (endedClipId != 0)
                OnClipEnded?.Invoke(endedClipId);
        }

        // 클립 종료 신호를 받았을 때
        private void EndSlot(SlotRuntime slot, bool immediate)
        {
            if (slot == null || !slot.occupied)
                return;

            float fadeOut = immediate ? 0f : Mathf.Max(0f, slot.currentEntry.fadeOut);
            StopSlot(slot, fadeOut);
        }

        private void MarkCancellable(SlotRuntime slot)
        {
            if (slot == null || !slot.occupied || slot.cancellable)
                return;

            slot.cancellable = true;
            OnClipCancellable?.Invoke(slot.currentClipId);
        }

        private bool IsSlotInterruptible(SlotRuntime slot)
        {
            if (slot == null || !slot.occupied)
                return true;

            if (slot.cancellable)
                return true;

            // fail-safe: cancellableAfter 시점이 지났으면 인터럽트 허용
            float clipLength = slot.currentEntry.clip != null ? slot.currentEntry.clip.length : 0f;
            if (clipLength <= 0f)
                return true;

            float normalized = (Time.time - slot.startTime) / clipLength;
            return normalized >= slot.currentEntry.cancellableAfter;
        }

        private void StartLifecycle(SlotRuntime slot)
        {
            CancelLifecycle(slot);
            slot.lifecycleCts = CancellationTokenSource.CreateLinkedTokenSource(_destroyToken);
            SlotLifecycleAsync(slot, slot.lifecycleCts.Token).Forget();
        }

        private void CancelLifecycle(SlotRuntime slot)
        {
            if (slot?.lifecycleCts == null)
                return;

            slot.lifecycleCts.Cancel();
            slot.lifecycleCts.Dispose();
            slot.lifecycleCts = null;
        }

        private async UniTaskVoid SlotLifecycleAsync(SlotRuntime slot, CancellationToken token)
        {
            var entry = slot.currentEntry;
            float clipLength = entry.clip != null ? entry.clip.length : 0f;
            int trackedClipId = slot.currentClipId;

            if (clipLength > 0f)
            {
                // cancellableAfter 시점에 도달하면 자동으로 cancellable 표시 (Animation Event 가 없을 때 fail-safe)
                float cancellableAt = clipLength * Mathf.Clamp01(entry.cancellableAfter);
                if (cancellableAt > 0f)
                {
                    await UniTask.Delay(TimeSpan.FromSeconds(cancellableAt), cancellationToken: token);
                    if (slot.currentClipId == trackedClipId && !slot.cancellable)
                        MarkCancellable(slot);
                }

                // 클립 종료 직전 페이드 아웃 시작
                float remain = clipLength - cancellableAt - entry.fadeOut;
                if (remain > 0f)
                    await UniTask.Delay(TimeSpan.FromSeconds(remain), cancellationToken: token);
            }

            // 동일 클립이 아직 살아있으면 종료 처리
            if (slot.currentClipId == trackedClipId && slot.occupied)
            {
                slot.lifecycleCts = null;
                EndSlot(slot, immediate: false);
            }
        }

        // ============================================================
        // 페이드 헬퍼
        // ============================================================

        private void StartFade(SlotRuntime slot, float target, float duration)
        {
            CancelFade(slot);
            slot.fadeCts = CancellationTokenSource.CreateLinkedTokenSource(_destroyToken);
            FadeLayerAsync(slot.config.layer, target, duration, slot.fadeCts.Token).Forget();
        }

        private void CancelFade(SlotRuntime slot)
        {
            if (slot?.fadeCts == null)
                return;

            slot.fadeCts.Cancel();
            slot.fadeCts.Dispose();
            slot.fadeCts = null;
        }

        private async UniTaskVoid FadeLayerAsync(int layer, float target, float duration, CancellationToken token)
        {
            if (_animator == null)
                return;

            float start = _animator.GetLayerWeight(layer);

            if (duration <= 0f)
            {
                _animator.SetLayerWeight(layer, target);
                return;
            }

            float t = 0f;
            while (t < duration)
            {
                if (token.IsCancellationRequested)
                    return;

                float w = Mathf.Lerp(start, target, t / duration);
                _animator.SetLayerWeight(layer, w);
                t += Time.deltaTime;
                await UniTask.Yield(PlayerLoopTiming.Update, token);
            }
            _animator.SetLayerWeight(layer, target);
        }

        // ============================================================
        // 보조
        // ============================================================

        private SlotRuntime GetSlot(MotionLayer layer)
        {
            if (_slots == null)
                return null;

            int idx = (int)layer;
            if (idx < 0 || idx >= _slots.Length)
                return null;

            return _slots[idx];
        }

        // ============================================================
        // Editor 전용 — placeholderClip 자동 동기화
        // ============================================================

#if UNITY_EDITOR
        // OnValidate: AnimatorController 의 각 슬롯 State 를 찾아서 motion(AnimationClip) 을 placeholderClip 으로 채운다.
        // - 두 군데에 같은 클립을 손으로 세팅해야 하던 불편을 제거한다.
        // - 에디터 전용. 런타임에는 영향 없음.
        private void OnValidate()
        {
            if (_animator == null)
                _animator = GetComponent<Animator>();

            if (_animator == null)
                return;

            var controller = _animator.runtimeAnimatorController as AnimatorController;
            if (controller == null)
                return;

            SyncPlaceholderFromController(controller, _fullSlotConfig);
            SyncPlaceholderFromController(controller, _upperSlotConfig);
            SyncPlaceholderFromController(controller, _lowerSlotConfig);
        }

        private static void SyncPlaceholderFromController(AnimatorController controller, SlotConfig config)
        {
            if (config == null || string.IsNullOrEmpty(config.stateName))
                return;

            if (controller.layers == null || config.layer < 0 || config.layer >= controller.layers.Length)
                return;

            var stateMachine = controller.layers[config.layer].stateMachine;
            if (stateMachine == null)
                return;

            var clip = FindStateMotion(stateMachine, config.stateName);
            if (clip != null && config.placeholderClip != clip)
                config.placeholderClip = clip;
        }

        private static AnimationClip FindStateMotion(AnimatorStateMachine stateMachine, string stateName)
        {
            if (stateMachine.states != null)
            {
                foreach (var s in stateMachine.states)
                {
                    if (s.state != null && s.state.name == stateName)
                        return s.state.motion as AnimationClip;
                }
            }

            // 서브 스테이트 머신 재귀 탐색
            if (stateMachine.stateMachines != null)
            {
                foreach (var sm in stateMachine.stateMachines)
                {
                    if (sm.stateMachine == null)
                        continue;

                    var clip = FindStateMotion(sm.stateMachine, stateName);
                    if (clip != null)
                        return clip;
                }
            }

            return null;
        }
#endif
    }
}
