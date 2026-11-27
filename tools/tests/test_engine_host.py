"""The DS engine (src/engine/llama2.c), built for the host, must reproduce the
golden outputs token for token. Needs a C compiler: run with `make tools-test`."""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import dslm  # noqa: E402

GOLDEN = dslm.GOLDEN_DIR / "stories260K.json"
# Generated tokens that must match runq.c, per packaged model (ADR 0008).
# stories260K: measured minimum 82. tiny1m (wider, dim 128): measured minimum 7
# ("Lily wanted to"); its stories stay coherent but leave runq.c's path early.
MIN_MATCH = {"stories260K": 64, "tiny1m": 6}


class EngineHost(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.exe = dslm.build_engine_binary()
        cls.golden = json.loads(GOLDEN.read_text(encoding="utf-8"))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_matches_golden_for_every_prompt(self):
        ckpt, tok = dslm.model_paths()
        steps = self.golden["steps"]
        for run in self.golden["runs"]:
            got = dslm.run_golden(self.exe, ckpt, tok, run["prompt"], steps)
            self.assertEqual(got["prompt_tokens"], run["prompt_tokens"], run["prompt"])
            self.assertEqual(got["tokens"], run["tokens"], run["prompt"])
            self.assertEqual(got["text"], run["text"], run["prompt"])

    def test_step_hook_forcing_top1_reproduces_greedy(self):
        """The visualizer's step hook (llama2.on_step) forcing topk's first token at
        temperature 1 must give the temperature-0 golden: the hook can steer, and
        topk ranks like the sampler's argmax."""
        import os
        q, tok = dslm.ensure_q80("stories260K")
        golden = json.loads((dslm.GOLDEN_DIR / "stories260K_q80.json").read_text(encoding="utf-8"))
        env = dict(os.environ, DSLM_HOOK_TOP1="1")
        for run in golden["runs"][:3]:
            out = subprocess.run([str(self.exe), str(q), str(tok), str(golden["steps"]), run["prompt"], "0", "1.0",
                                  "0.9", "7"], check=True, capture_output=True, text=True, env=env).stdout
            tokens = [int(t) for t in next(l for l in out.splitlines() if l.startswith("T ")).split()[1:]]
            self.assertEqual(tokens, run["tokens"], run["prompt"])

    def test_stepwise_continuation_equals_uninterrupted(self):
        """Choose-your-path continues a finished run with llama2_next from its last token:
        stopping after k tokens and stepping on must give the golden sequence exactly."""
        import os
        for model in ("stories260K",):
            q, tok = dslm.ensure_q80(model)
            golden = json.loads((dslm.GOLDEN_DIR / f"{model}_q80.json").read_text(encoding="utf-8"))
            env = dict(os.environ, DSLM_CONTINUE="1")
            for run, stop_after in zip(golden["runs"][:3], (1, 17, 60)):
                out = subprocess.run([str(self.exe), str(q), str(tok), str(golden["steps"]), run["prompt"],
                                      str(stop_after)], check=True, capture_output=True, text=True, env=env).stdout
                tokens = [int(t) for t in next(l for l in out.splitlines() if l.startswith("T ")).split()[1:]]
                self.assertEqual(tokens, run["tokens"], (model, run["prompt"], stop_after))

    def test_int8_matches_its_golden_exactly(self):
        """The DS reference: the fixed-point engine is integer-only, so any change shows here."""
        for model in MIN_MATCH:
            q, tok = dslm.ensure_q80(model)
            golden = json.loads((dslm.GOLDEN_DIR / f"{model}_q80.json").read_text(encoding="utf-8"))
            for run in golden["runs"]:
                got = dslm.run_golden(self.exe, q, tok, run["prompt"], golden["steps"])
                self.assertEqual(got["tokens"], run["tokens"], (model, run["prompt"]))
                self.assertEqual(got["text"], run["text"], (model, run["prompt"]))

    def test_int8_drift_against_runq_is_bounded(self):
        """Quality: the fixed-point engine follows float runq.c for at least MIN_MATCH generated tokens."""
        for model, bound in MIN_MATCH.items():
            q, tok = dslm.ensure_q80(model)
            ref = json.loads((dslm.GOLDEN_DIR / f"{model}_q80_runq.json").read_text(encoding="utf-8"))
            for run in ref["runs"]:
                got = dslm.run_golden(self.exe, q, tok, run["prompt"], ref["steps"])
                n, bad = dslm.compare_tokens(got["tokens"], run["tokens"])
                matching = (n if bad is None else bad) - run["prompt_tokens"]
                self.assertGreaterEqual(matching, min(bound, len(run["tokens"]) - run["prompt_tokens"]),
                                        (model, run["prompt"]))

    def sample_run(self, ckpt, tok, temperature, topp, seed):
        out = subprocess.run([str(self.exe), str(ckpt), str(tok), "48", "Once upon a time", "0",
                              str(temperature), str(topp), str(seed)],
                             check=True, capture_output=True, text=True).stdout
        return [int(t) for t in out.splitlines()[1].split()[1:]]

    def test_int8_sampling_is_reproducible_and_valid(self):
        q, tok = dslm.ensure_q80()
        for topp in ("0.9", "1.0"):  # nucleus and plain multinomial
            a = self.sample_run(q, tok, "1.0", topp, 42)
            b = self.sample_run(q, tok, "1.0", topp, 42)
            c = self.sample_run(q, tok, "1.0", topp, 7)
            self.assertEqual(a, b, topp)                 # same seed, same story
            self.assertNotEqual(a, c, topp)              # another seed, another story
            self.assertTrue(all(0 <= t < 512 for t in a))
            self.assertEqual(a[:5], [1, 403, 407, 261, 378])  # the prompt itself is forced

    def test_stop_keeps_the_tokens_produced_so_far(self):
        ckpt, tok = dslm.model_paths()
        run = self.golden["runs"][1]
        out = subprocess.run([str(self.exe), str(ckpt), str(tok), "64", run["prompt"], "10"],
                             check=True, capture_output=True, text=True).stdout
        tokens = [int(t) for t in out.splitlines()[1].split()[1:]]
        self.assertEqual(len(tokens), 11)  # BOS + the 10 tokens seen before stopping
        self.assertEqual(tokens, run["tokens"][:11])

    def test_rejects_a_prompt_longer_than_the_buffer(self):
        ckpt, tok = dslm.model_paths()
        r = subprocess.run([str(self.exe), str(ckpt), str(tok), "4", "a" * 300], capture_output=True)
        self.assertNotEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()
