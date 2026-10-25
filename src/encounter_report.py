"""Extract encounter evidence without promoting observations into coverage claims."""
import json
import re
from pathlib import Path

PREFIXES = ('boss-run ', 'boss-stage ', 'boss-stage-result ', 'boss-animation-ids ',
            'compilation evidence:', 'cutscene ', 'skipped lamp ', 'area ', 'lamp tour finished')

def summarize(log_text):
    events = []
    for line in log_text.splitlines():
        if '[warper] ' not in line:
            continue
        message = line.split('[warper] ', 1)[1]
        if message.startswith(PREFIXES):
            events.append(message)
    results = []
    for message in events:
        if not message.startswith('boss-stage-result '):
            continue
        fields = dict(re.findall(r'(boss|stage|animations|pipelines|compiles|hp)=([^ ;]+)', message))
        for key in ('animations', 'pipelines', 'compiles', 'hp'):
            if key in fields:
                fields[key] = int(fields[key])
        results.append(fields)
    return {'scope': 'this log session, including automatic retries',
            'coverage_warning': 'HP values, loaded entities and animation IDs do not prove boss phases, specific attacks or exhaustive shader coverage.',
            'tour_finished': any(e.startswith('lamp tour finished') for e in events),
            'stage_results': results, 'events': events}

def write_report(log_path, output_path):
    report = summarize(Path(log_path).read_text(encoding='utf-8', errors='replace'))
    Path(output_path).write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report
