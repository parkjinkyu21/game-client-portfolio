using System;
using System.Collections.Generic;
using UnityEngine;

namespace PJ.Core.Events
{
    // 전역 이벤트 버스. EventKey 기반의 발행-구독(Pub-Sub) 패턴을 제공한다.
    // - EventKey 로 식별되는 이벤트에 Delegate 를 등록(Register)하고, 실행(Execute) 시 호출한다.
    // - 제네릭 오버로딩으로 0~5 개 인자를 타입 안전하게 전달할 수 있다.
    // - 동일 핸들러의 중복 등록을 방지한다.
    // - 모듈 간 직접 참조 없이 이벤트로 통신할 수 있어 디커플링에 사용한다.
    public class EventController : Singleton<EventController>
    {
        private readonly Dictionary<EventKey, Delegate> _eventTable = new();

        private void OnDisable()
        {
            _eventTable.Clear();
        }

        // 중복 등록을 방지하며 이벤트 핸들러를 등록한다.
        private void RegisterThisEvent(EventKey eventName, Delegate handler)
        {
            if (_eventTable.TryGetValue(eventName, out Delegate prevHandlers))
            {
                if (prevHandlers != null)
                {
                    var list = prevHandlers.GetInvocationList();
                    for (int i = 0; i < list.Length; i++)
                    {
                        if (list[i].Equals(handler))
                            return;
                    }
                }

                _eventTable[eventName] = Delegate.Combine(prevHandlers, handler);
            }
            else
            {
                _eventTable.Add(eventName, handler);
            }
        }
        public void RegisterEvent(EventKey eventName, Action handler) => RegisterThisEvent(eventName, handler);
        public void RegisterEvent<T1>(EventKey eventName, Action<T1> handler) => RegisterThisEvent(eventName, handler);
        public void RegisterEvent<T1, T2>(EventKey eventName, Action<T1, T2> handler) => RegisterThisEvent(eventName, handler);
        public void RegisterEvent<T1, T2, T3>(EventKey eventName, Action<T1, T2, T3> handler) => RegisterThisEvent(eventName, handler);
        public void RegisterEvent<T1, T2, T3, T4>(EventKey eventName, Action<T1, T2, T3, T4> handler) => RegisterThisEvent(eventName, handler);
        public void RegisterEvent<T1, T2, T3, T4, T5>(EventKey eventName, Action<T1, T2, T3, T4, T5> handler) => RegisterThisEvent(eventName, handler);

        // 이벤트 핸들러를 제거한다. 제거 후 딕셔너리에 빈 엔트리가 남지 않도록 정리한다.
        private void UnregisterThisEvent(EventKey eventName, Delegate handler)
        {
            if (_eventTable.TryGetValue(eventName, out Delegate prevHandlers))
            {
                Delegate result = Delegate.Remove(prevHandlers, handler);
                if (result == null)
                    _eventTable.Remove(eventName);
                else
                    _eventTable[eventName] = result;
            }
        }
        public void UnregisterEvent(EventKey eventName, Action handler) => UnregisterThisEvent(eventName, handler);
        public void UnregisterEvent<T1>(EventKey eventName, Action<T1> handler) => UnregisterThisEvent(eventName, handler);
        public void UnregisterEvent<T1, T2>(EventKey eventName, Action<T1, T2> handler) => UnregisterThisEvent(eventName, handler);
        public void UnregisterEvent<T1, T2, T3>(EventKey eventName, Action<T1, T2, T3> handler) => UnregisterThisEvent(eventName, handler);
        public void UnregisterEvent<T1, T2, T3, T4>(EventKey eventName, Action<T1, T2, T3, T4> handler) => UnregisterThisEvent(eventName, handler);
        public void UnregisterEvent<T1, T2, T3, T4, T5>(EventKey eventName, Action<T1, T2, T3, T4, T5> handler) => UnregisterThisEvent(eventName, handler);

        private Delegate GetDelegate(EventKey eventName)
        {
            if (_eventTable.TryGetValue(eventName, out Delegate handlers))
            {
                return handlers;
            }
            return null;
        }
        public void ExecuteEvent(EventKey eventName)
        {
            if (GetDelegate(eventName) is Action action)
                action();
        }
        public void ExecuteEvent<T1>(EventKey eventName, T1 t1)
        {
            if (GetDelegate(eventName) is Action<T1> action)
                action(t1);
        }
        public void ExecuteEvent<T1, T2>(EventKey eventName, T1 t1, T2 t2)
        {
            if (GetDelegate(eventName) is Action<T1, T2> action)
                action(t1, t2);
        }
        public void ExecuteEvent<T1, T2, T3>(EventKey eventName, T1 t1, T2 t2, T3 t3)
        {
            if (GetDelegate(eventName) is Action<T1, T2, T3> action)
                action(t1, t2, t3);
        }
        public void ExecuteEvent<T1, T2, T3, T4>(EventKey eventName, T1 t1, T2 t2, T3 t3, T4 t4)
        {
            if (GetDelegate(eventName) is Action<T1, T2, T3, T4> action)
                action(t1, t2, t3, t4);
        }
        public void ExecuteEvent<T1, T2, T3, T4, T5>(EventKey eventName, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
        {
            if (GetDelegate(eventName) is Action<T1, T2, T3, T4, T5> action)
                action(t1, t2, t3, t4, t5);
        }
    }
}
