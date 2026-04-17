using UnityEngine;

namespace PJ.Core.Input
{
    // InputController 가 관리하는 모든 InputReader 의 공통 베이스 클래스.
    //
    // - Inspector 에서 참조/등록하기 위해 MonoBehaviour 를 상속한다.
    // - 제네릭 타입은 Unity 에서 직렬화되지 않기 때문에,
    //   Inspector 노출 및 컨트롤러 관리를 위해 non-generic 베이스를 분리했다.
    // - Core 레이어는 입력의 의미(이동, 공격 등)를 알지 않기 때문에
    //   이 클래스에는 어떤 입력 처리 메서드도 정의하지 않는다.
    //
    // InputReader 의 역할:
    // - Unity Input System 콜백을 수신한다.
    // - 활성화/비활성화 및 생명주기를 관리한다.
    // - 바인딩된 InputHandler 로 입력을 전달한다.
    //
    // 실제 입력 의미는 App 레이어의 구체 Reader 에서 정의된다.
    public abstract class InputReaderBase : MonoBehaviour
    {
        // InputReader 가 활성화될 때 호출된다.
        // InputAction 생성, 콜백 등록 등의 초기화를 수행한다.
        public abstract void Initialize();

        // InputReader 가 비활성화되거나 교체될 때 호출된다.
        // InputAction 해제, 콜백 제거 등의 정리를 수행한다.
        // Dispose 이후 다시 사용하려면 Initialize 를 다시 호출해야 한다.
        public abstract void Dispose();

        // InputReader 를 완전히 제거하지 않고
        // 입력 처리만 활성/비활성화할 때 사용한다.
        public abstract void SetEnabled(bool enabled);

        // InputReader 에 입력을 처리할 Handler 를 바인딩한다.
        // 실제 타입 검증은 제네릭 서브 클래스에서 수행된다.
        public abstract void BindHandler(InputHandlerBase handler);

        // 현재 바인딩된 InputHandler 를 해제한다.
        public abstract void UnbindHandler();
    }

    // 특정 InputHandler 타입과 바인딩되기 위한 제네릭 InputReader 베이스 클래스.
    //
    // - Handler 타입에 대한 캐스팅 및 검증 로직을 공통화한다.
    // - 구체 InputReader 에서는 캐스팅 없이 InputHandler 를 바로 사용할 수 있다.
    //
    // 주의:
    // - 이 제네릭 클래스는 Unity 에서 직렬화되지 않는다.
    // - Inspector 참조 및 관리에는 반드시 non-generic InputReaderBase 를 사용해야 한다.
    //
    // 사용 예:
    //     class AppInputReader : InputReaderBase<AppInputHandler>
    public abstract class InputReaderBase<THandler> : InputReaderBase
        where THandler : InputHandlerBase
    {
        // 현재 바인딩된 InputHandler.
        // BindHandler 가 성공한 경우에만 유효하다.
        public THandler InputHandler { get; private set; }

        // InputHandler 를 바인딩한다.
        // 전달된 Handler 가 기대 타입과 다를 경우 에러 로그를 출력한다.
        public override void BindHandler(InputHandlerBase baseHandler)
        {
            InputHandler = baseHandler as THandler;
            if (InputHandler == null)
            {
                Debug.LogError(
                    $"[InputReader] Bind FAILED. Expected: {typeof(THandler).Name}, " +
                    $"Got: {baseHandler?.GetType().Name ?? "null"}"
                );
                return;
            }

            Debug.Log(
                $"[InputReader] Bind SUCCESS. Handler Type: {InputHandler.GetType().Name}"
            );
        }

        // 현재 바인딩된 InputHandler 를 해제한다.
        public override void UnbindHandler()
        {
            if (InputHandler != null)
            {
                Debug.Log(
                    $"[InputReader] Unbind. Previous Handler Type: {InputHandler.GetType().Name}"
                );
            }
            else
            {
                Debug.Log(
                    "[InputReader] Unbind called, but handler was already null"
                );
            }

            InputHandler = null;
        }
    }
}
