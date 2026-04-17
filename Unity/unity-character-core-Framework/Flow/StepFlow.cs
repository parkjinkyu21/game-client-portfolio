using System;
using System.Collections.Generic;
using UnityEngine;

namespace PJ.Core.Flow
{
    // 순차 흐름(Step) 관리자. 빌더 패턴으로 단계별 흐름을 선언적으로 구성한다.
    // - Step → Wait → Complete/Fail/Cancel 순서로 각 단계를 정의한다.
    // - Update 를 매 프레임 호출하면 현재 Step 의 Wait 조건을 평가하여 진행한다.
    // - Wait 없이 Step 만 등록하면 Start 호출 직후 즉시 Complete 로 넘어간다.
    // - ForceFail / ForceCancel 로 외부에서 흐름을 강제 중단할 수 있다.
    //
    // 사용 예:
    //   new StepFlow()
    //       .Step("Load", () => StartLoad())
    //       .Wait(() => isLoaded ? StepWaitResult.Success : StepWaitResult.Waiting)
    //       .Complete(() => Debug.Log("Done"))
    //       .OnCompleted(() => Debug.Log("All done"))
    //       .Start();
    public class StepFlow
    {
        public enum StepWaitResult
        {
            Waiting,
            Success,
            Failed,
            Cancelled
        }

        public enum FlowState
        {
            Idle,
            Running,
            Completed,
            Failed,
            Cancelled
        }

        private class StepNode
        {
            public string Name;

            public Action OnStart;
            public Func<StepWaitResult> OnWait;

            public Action OnComplete;
            public Action OnFail;
            public Action OnCancel;

            public bool IsStarted;
        }

        private readonly List<StepNode> _steps = new();

        private int _index;

        public FlowState State { get; private set; } = FlowState.Idle;

        private Action _onFlowComplete;
        private Action _onFlowFail;
        private Action _onFlowCancel;

        private void Log(string msg)
        {
            Debug.Log($"[StepFlow] {msg}");
        }

        private void LogError(string msg)
        {
            Debug.LogError($"[StepFlow] {msg}");
        }

        private StepNode LastStep()
        {
            if (_steps.Count == 0)
            {
                LogError("Step 이 없습니다. Step() 을 먼저 호출하세요.");
                return null;
            }

            return _steps[^1];
        }

        public StepFlow Step(string name, Action start)
        {
            _steps.Add(new StepNode
            {
                Name = name,
                OnStart = start
            });

            return this;
        }

        public StepFlow Wait(Func<StepWaitResult> wait)
        {
            var step = LastStep();
            if (step == null)
                return this;

            step.OnWait = wait;

            return this;
        }

        public StepFlow Complete(Action complete)
        {
            var step = LastStep();
            if (step == null)
                return this;

            step.OnComplete = complete;

            return this;
        }

        public StepFlow Fail(Action fail)
        {
            var step = LastStep();
            if (step == null)
                return this;

            step.OnFail = fail;

            return this;
        }

        public StepFlow Cancel(Action cancel)
        {
            var step = LastStep();
            if (step == null)
                return this;

            step.OnCancel = cancel;

            return this;
        }

        public StepFlow OnCompleted(Action complete)
        {
            _onFlowComplete = complete;
            return this;
        }

        public StepFlow OnFailed(Action fail)
        {
            _onFlowFail = fail;
            return this;
        }

        public StepFlow OnCancelled(Action cancel)
        {
            _onFlowCancel = cancel;
            return this;
        }

        public void Start()
        {
            if (_steps.Count == 0)
            {
                LogError("StepFlow 시작 실패: Step 이 없음");
                return;
            }

            foreach (var step in _steps)
            {
                step.IsStarted = false;
            }

            _index = 0;

            State = FlowState.Running;

            Log("Flow Started");
        }

        public void Update()
        {
            if (State != FlowState.Running)
                return;

            if (_index >= _steps.Count)
            {
                State = FlowState.Completed;
                Log("Flow Completed");

                _onFlowComplete?.Invoke();
                return;
            }

            var step = _steps[_index];

            if (!step.IsStarted)
            {
                step.IsStarted = true;

                Log($"Step Start : {step.Name}");

                step.OnStart?.Invoke();

                if (step.OnWait == null)
                {
                    step.OnComplete?.Invoke();
                    _index++;
                    return;
                }
            }

            var result = step.OnWait();

            switch (result)
            {
                case StepWaitResult.Waiting:
                    break;

                case StepWaitResult.Success:

                    Log($"Step Complete : {step.Name}");

                    step.OnComplete?.Invoke();

                    _index++;

                    break;

                case StepWaitResult.Failed:

                    LogError($"Step Failed : {step.Name}");

                    step.OnFail?.Invoke();

                    State = FlowState.Failed;

                    _onFlowFail?.Invoke();

                    break;

                case StepWaitResult.Cancelled:

                    Log($"Step Cancelled : {step.Name}");

                    step.OnCancel?.Invoke();

                    State = FlowState.Cancelled;

                    _onFlowCancel?.Invoke();

                    break;
            }
        }

        public void ForceFail()
        {
            if (State != FlowState.Running)
                return;

            LogError("Flow Force Failed");

            State = FlowState.Failed;

            _onFlowFail?.Invoke();
        }

        public void ForceCancel()
        {
            if (State != FlowState.Running)
                return;

            Log("Flow Force Cancelled");

            State = FlowState.Cancelled;

            _onFlowCancel?.Invoke();
        }
    }
}
