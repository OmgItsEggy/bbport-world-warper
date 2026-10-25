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

    def test_missing_and_scripted_bosses_are_not_certified(self):
        report=summarize('[warper] boss-roster boss=Ludwig previous-attempt=no special=no\n'
                         '[warper] boss-roster boss=Witches previous-attempt=no special=group-or-scripted\n'
                         '[warper] boss-attempt boss=Witches result=scripted-observation-only; NOT exhaustive coverage\n')
        self.assertEqual(report['boss_roster']['Ludwig']['status'],'not-attempted-this-session')
        self.assertEqual(report['boss_roster']['Witches']['status'],'scripted-observation-only')

    def test_blocked_ending_entry_has_explicit_status(self):
        report=summarize('[warper] boss-roster boss=Gehrman previous-attempt=no special=no\n'
                         '[warper] boss-unavailable boss=Gehrman reason=ending-arena-entry-not-implemented; no ordinary Dream warp substituted\n')
        self.assertEqual(report['boss_roster']['Gehrman']['status'],'unavailable-ending-arena-entry-not-implemented')
