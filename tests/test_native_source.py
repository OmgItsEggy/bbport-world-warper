import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "ports/supermedo"


class NativeSourceContractTests(unittest.TestCase):
    def test_runtime_pad_exports_director_input(self):
        header = (PORT / "src/runtime.h").read_text(encoding="utf-8")
        source = (PORT / "src/runtime_pad.c").read_text(encoding="utf-8")
        self.assertIn("runtime_pad_director_press", header)
        self.assertIn("runtime_pad_director_sticks", header)
        self.assertIn("injected.buttons|director_buttons", source)
        self.assertIn("if (director_buttons & BTN_L2)", source)
        self.assertIn("if (v>=0) *axes[i]", source)

    def test_memory_access_uses_runtime_fault_recovery(self):
        source = (PORT / "gpu/shim/bbport_world_warper_memory.cpp").read_text(encoding="utf-8")
        self.assertGreaterEqual(source.count("runtime_fault_recover = &recovery"), 2)
        self.assertGreaterEqual(source.count("runtime_fault_recover = previous"), 4)
        self.assertIn("sigsetjmp(recovery, 1)", source)
        self.assertIn("address + size >= address", source)

    def test_image_binding_checks_bounds_and_original_bytes(self):
        header = (PORT / "gpu/shim/bbport_world_warper_image.h").read_text(encoding="utf-8")
        source = (PORT / "gpu/shim/bbport_world_warper_image.cpp").read_text(encoding="utf-8")
        self.assertIn("bool Bind(unsigned char* image", header)
        self.assertIn("bool Matches(std::uint64_t offset", header)
        self.assertIn("length <= size - offset", source)
        self.assertIn("compare_exchange_strong", source)
        self.assertIn("image[offset + index] != byte", source)


if __name__ == "__main__":
    unittest.main()

