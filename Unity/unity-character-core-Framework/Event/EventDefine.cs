
namespace PJ.Core.Events
{
    // 이벤트 식별자. 문자열 기반 값 객체로, Dictionary 키 성능을 위해 IEquatable 을 구현한다.
    public readonly struct EventKey : System.IEquatable<EventKey>
    {
        public readonly string Value;

        public EventKey(string value)
        {
            Value = value;
        }

        public bool Equals(EventKey other) => string.Equals(Value, other.Value, System.StringComparison.Ordinal);
        public override bool Equals(object obj) => obj is EventKey other && Equals(other);
        public override int GetHashCode() => Value != null ? Value.GetHashCode() : 0;

        public static bool operator ==(EventKey left, EventKey right) => left.Equals(right);
        public static bool operator !=(EventKey left, EventKey right) => !left.Equals(right);
    }

    // 하위 다른 모듈에서 필요한 EventKey 추가해서 사용한다.
    public static partial class EventKeys
    {
        public static readonly EventKey EX_Key1 = new EventKey("EX/Key1");
        public static readonly EventKey EX_Key2 = new EventKey("EX/Key2");
    }
}
