using UnityEngine;

namespace PJ.Core.Actor
{
    // 현재 장착 파츠로 인해 비활성화해야 하는 캐릭터 렌더 파트를 정의하는 컴포넌트이다.
    public class CharacterEquipPartsHideRule : MonoBehaviour
    {
        [SerializeField] private CharacterRenderPart[] _hiddenRenderParts;
        public CharacterRenderPart[] HiddenRenderParts => _hiddenRenderParts;
    }
}
