using UnityEngine;

namespace PJ.Core.Input
{
    // InputReader 와 바인딩되는 입력 처리자의 공통 베이스 클래스.
    //
    // - 실제 입력 의미(OnMove, OnTap 등)는 App 레이어에서 정의된다.
    // - Core 레이어는 입력의 종류나 의미를 알지 않기 위해
    //   이 클래스에는 어떤 입력 메서드도 정의하지 않는다.
    // - MonoBehaviour 를 상속받는 이유는
    //   씬 상의 컴포넌트로 배치하고 Inspector/라이프사이클을
    //   이용해 바인딩하기 위함이다.
    //
    // 이 클래스는 "입력 처리자"의 식별자 역할만 하며,
    // 의도적으로 비어 있는 타입이다.
    public class InputHandlerBase : MonoBehaviour
    {
    }
}
