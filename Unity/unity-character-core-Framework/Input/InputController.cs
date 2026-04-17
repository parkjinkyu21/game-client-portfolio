using UnityEngine;

namespace PJ.Core.Input
{
    // 입력 시스템의 진입점. 활성 InputReader 를 관리하고 교체한다.
    // - 기본 Reader 를 인스펙터에서 설정하고, 런타임에 SwitchTo 로 교체할 수 있다.
    // - Reader 교체 시 이전 Reader 를 Dispose 하고 새 Reader 를 Initialize 한다.
    // - Handler 바인딩은 Reader 교체 후 호출자가 직접 수행해야 한다.
    // - Core 레이어는 입력의 의미(이동, 공격 등)를 알지 않으며,
    //   플랫폼별 Reader 와 App 레이어의 Handler 사이를 중개하는 역할만 한다.
    public class InputController : Singleton<InputController>
    {
        [SerializeField] private InputReaderBase _defaultReader;
        public InputReaderBase DefaultReader => _defaultReader;
        private InputReaderBase _current;

        private void Start()
        {
            SwitchTo(_defaultReader);
        }

        public void SetDefaultReader(InputReaderBase reader)
        {
            if (_defaultReader != reader)
            {
                _defaultReader = reader;
                SwitchTo(_defaultReader);
            }
        }

        // Reader 교체 시 이전 Reader 를 정리하고 새 Reader 를 초기화한다.
        // 교체 후에는 BindHandler 를 다시 호출해야 한다.
        public void SwitchTo(InputReaderBase reader)
        {
            _current?.Dispose();
            _current = reader;
            _current?.Initialize();
        }

        public void SetEnabled(bool enabled)
        {
            if (enabled) _current?.Initialize();
            else _current?.Dispose();
        }

        public void BindHandler(InputHandlerBase handler)
        {
            _current?.BindHandler(handler);
        }

        public void UnbindHandler()
        {
            _current?.UnbindHandler();
        }
    }
}
