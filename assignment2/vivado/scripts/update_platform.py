"""Run with `vitis -s scripts/update_platform.py`; rebuild software platform only."""
from pathlib import Path
import vitis

root = Path(__file__).resolve().parents[1]
client = vitis.create_client(workspace=str(root))
try:
    platform = client.get_component(name='platform')
    platform.update_hw(hw_design=str(root / 'artifacts/dfx/main_wrapper.xsa'))
    result = platform.build()
    print('PLATFORM_BUILD_RESULT', result)
    if result != 0:
        raise RuntimeError(f'Vitis platform build failed: {result}')
finally:
    vitis.dispose()
