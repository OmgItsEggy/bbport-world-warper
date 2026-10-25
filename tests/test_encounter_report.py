import unittest
from src.encounter_report import summarize

class EncounterReportTests(unittest.TestCase):
    def test_stage_evidence_is_not_promoted_to_attack_coverage(self):
        report=summarize('[warper] boss-stage-result boss=Logarius stage=ranged animations=4 pipelines=7 compiles=13 hp=1000; animation IDs are not attack/phase classification\n'
                         '[warper] boss-stage boss=Logarius stage=close range=3 placement=FAILED hp-percent=100 hp-request=unchanged\n'
                         '[warper] lamp tour finished; see done/skip progress for coverage\n')
        self.assertTrue(report['tour_finished'])
        self.assertEqual(report['stage_results'][0]['pipelines'],7)
        self.assertEqual(report['stage_results'][0]['stage'],'ranged')
        self.assertIn('do not prove',report['coverage_warning'])
        self.assertIn('placement=FAILED',report['events'][1])

    def test_empty_or_interrupted_session_is_not_complete(self):
        self.assertFalse(summarize('GPU: device lost')['tour_finished'])
        self.assertEqual(summarize('')['stage_results'],[])
