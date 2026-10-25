"""One command: equipment and all lanterns, then boss/phase encounters."""
from tools.run_smoke import main as run_tour


def run_full(arguments):
    results = []
    for label, mode in (('WEAPONS / ARMOUR + 44 LANTERNS', '--all-lamps'),
                        ('BOSSES / PHASES', '--bosses')):
        print('\n=== FULL WARM-UP: ' + label + ' ===', flush=True)
        results.append(run_tour([*arguments, mode]))
    print('\nFull warm-up attempts finished. Check all-lamps-report.json and bosses-report.json.', flush=True)
    print('Incomplete/unsupported encounters are not counted as confirmed coverage.', flush=True)
    return 1 if any(results) else 0
