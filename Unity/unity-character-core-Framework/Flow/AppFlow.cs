using UnityEngine;

namespace PJ.Core.Flow
{
    // 앱 전역 상태 머신. AppStateBase 를 상태 단위로 관리하고 전환한다.
    // - StartFlow 로 최초 상태를 설정하고, ChangeState 로 전환을 요청한다.
    // - OnEnter 실행 중 다른 ChangeState 가 호출되면 _pending 에 버퍼링하여
    //   다음 Update 에서 안전하게 전환한다.
    // - 현재 상태의 Update 는 전환 요청이 없는 프레임에서만 호출된다.
    public class AppFlow : Singleton<AppFlow>
    {
        private AppStateBase _current;
        private AppStateBase _pending;

        public AppStateBase Current => _current;

        public void StartFlow(AppStateBase start)
        {
            if (start == null)
            {
                Debug.LogError("[AppFlow] StartFlow 에 null 상태가 전달되었습니다.");
                return;
            }

            ChangeState(start);
        }

        // 외부 API
        public void ChangeState(AppStateBase next)
        {
            if (next == null)
            {
                Debug.LogError("[AppFlow] ChangeState 에 null 상태가 전달되었습니다.");
                return;
            }

            _pending = next;

            // Enter 중이 아니면 즉시 전환 시도
            TrySwitchState();
        }

        private void Update()
        {
            // 예약된 상태가 있으면 전환
            if (_pending != null)
            {
                TrySwitchState();
            }
            else
            {
                _current?.Update(Time.deltaTime);
            }
        }

        private void TrySwitchState()
        {
            var next = _pending;
            _pending = null;

            _current?.OnExit(this);
            _current = next;
            _current?.OnEnter(this);
        }
    }
}
