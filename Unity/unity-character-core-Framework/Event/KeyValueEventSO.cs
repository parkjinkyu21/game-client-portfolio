using System;
using UnityEngine;

namespace PJ.Core.Events
{
    // ScriptableObject 기반 제네릭 이벤트 채널.
    // - 에디터 인스펙터에서 이벤트를 연결할 수 있어 EventController 를 거치지 않는 직접 통신에 사용한다.
    // - Raise 로 이벤트를 발행하고, OnEventRaised 에 구독한다.
    // - event 키워드를 사용하여 외부에서 += / -= 만 허용하고, 직접 대입이나 null 초기화를 방지한다.
    public class KeyValueEventSO<T> : ScriptableObject
    {
        public event Action<T> OnEventRaised;
        public void Raise(T value)
        {
            OnEventRaised?.Invoke(value);
        }
    }
}
