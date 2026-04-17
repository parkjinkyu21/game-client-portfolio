using System;

namespace PJ.Core.Actor
{
    // 게임 로직(App 레이어의 행동 컴포넌트)이 보는 View 인터페이스.
    // - View 구현(CharacterAnimator) 의 구체 타입에 게임 로직이 결합되지 않도록 하기 위함.
    // - 클립 데이터(MotionClipTable.Entry) 는 호출자가 조회해서 전달한다. View 는 테이블을 소유하지 않는다.
    // - 이벤트 통지는 clipId 단위로 이루어지며, 게임 로직은 clipId 로 역번역해서 사용한다.
    public interface IMotionPlayer
    {
        // 이 엔트리를 지금 재생할 수 있는지 검사한다.
        // - 슬롯이 비어 있거나, 현재 점유 중인 클립이 인터럽트 가능 윈도우에 있으면 true.
        // - entry.clip 이 null 이거나 clipId 가 0 이면 false.
        bool CanPlay(in MotionClipTable.Entry entry);

        // 엔트리를 실제로 재생한다. 성공하면 true.
        // - CanPlay 가 false 면 재생하지 않고 false 를 반환한다.
        bool Play(in MotionClipTable.Entry entry);

        // 모든 슬롯을 페이드 아웃하며 정지한다.
        void StopAll(float fadeOut = 0.15f);

        // 클립 재생이 시작된 시점 (slot swap + 페이드 인 시작)
        event Action<int> OnClipStarted;

        // 인터럽트 윈도우에 진입했을 때
        // - Animation Event 마킹이 발생했거나, fail-safe 시간(cancellableAfter) 이 도달했을 때 발행
        event Action<int> OnClipCancellable;

        // 클립 재생이 종료된 시점 (페이드 아웃 완료)
        event Action<int> OnClipEnded;
    }
}
