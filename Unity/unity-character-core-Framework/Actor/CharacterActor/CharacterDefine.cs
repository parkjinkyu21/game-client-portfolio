using UnityEngine;

namespace PJ.Core.Actor
{
    // 캐릭터 바디를 부위별로 분할한 렌더 파츠
    public enum CharacterRenderPart
    {
        Head,
        Torso,
        Arms,
        Legs,
        Feet,
        Hands,
    }

    // 캐릭터 장비의 장착 부위
    public enum CharacterEquipPart
    {
        Top,
        Bottom,
        Shoes,
    }

    // 모션 클립이 재생될 Animator 오버레이 레이어 지정
    // - Upper / Lower : 각각 상체 / 하체 오버레이 레이어 (Avatar Mask 로 영역 분리, 서로 공존 가능)
    // - Full          : 전신 오버레이 레이어 (Upper/Lower 와 상호 배타)
    // Base 레이어(로코모션 State Machine) 는 이 enum 에 포함되지 않는다. 항상 weight 1 로 돌아가며
    // 클립 스왑 시스템이 건드리지 않는다. 위 오버레이들이 각자의 마스크 영역만 Base 위에 얹는다.
    public enum MotionLayer
    {
        Upper = 0,
        Lower = 1,
        Full  = 2,
    }
}
