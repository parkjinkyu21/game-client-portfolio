using UnityEngine;

namespace PJ.Core.Actor
{
    // 씬에 배치되는 모든 액터의 기본 추상 클래스.
    // - Guid 로 식별하고 Transform, 활성화 상태를 관리한다.
    // - 구체 액터(CharacterActor 등)는 이 클래스를 상속하여 고유 로직을 추가한다.
    public abstract class Actor : MonoBehaviour
    {
        public string Guid { get; private set; }
        public Transform Root => transform;

        public bool IsActive => gameObject.activeSelf;

        public virtual void SetActive(bool active)
        {
            gameObject.SetActive(active);
        }

        public virtual void Initialize(string id)
        {
            Guid = id;
        }
    }
}
