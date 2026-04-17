namespace PJ.Core.Flow
{
    // AppFlow 에서 관리하는 앱 상태의 추상 기본 클래스.
    // - OnEnter/OnExit 는 internal 로 제한하여 AppFlow 만 호출할 수 있다.
    // - App 레이어에서 이 클래스를 상속하여 Lobby, InGame 등 구체 상태를 정의한다.
    public abstract class AppStateBase
    {
        // 상태 진입
        internal abstract void OnEnter(AppFlow flow);

        // 상태 종료
        internal virtual void OnExit(AppFlow flow) { }

        // 안정 상태 로직
        public virtual void Update(float deltaTime) { }
    }
}
