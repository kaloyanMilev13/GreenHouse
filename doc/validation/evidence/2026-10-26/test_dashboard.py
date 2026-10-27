"""Run actual dashboard SQL on a disposable standalone PostgreSQL cluster.
No network listener and no production database access. Fixture tables only.
Requires initdb/postgres in PATH. PostgreSQL's single-user mode executes SQL.
"""
from pathlib import Path
import json, subprocess, tempfile, re, hashlib
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
source=ROOT/'dashboard/greenhouse-overview.json'
dashboard=json.loads(source.read_text())
panels={p['id']:p for p in dashboard['panels']}
statements=[]; checks=[]
def add(s):statements.append(re.sub(r'\s+',' ',s).strip())
def query(pid,mode='Real'):
    s=panels[pid]['targets'][0]['rawSql'].strip().rstrip(';')
    # Grafana's two documented macro inputs are the only substitutions.
    return s.replace('${mode:sqlstring}',"'"+mode+"'").replace('$__timeFilter(received_at)',"received_at BETWEEN now()-interval '1 day' AND now()+interval '1 hour'")
def check(label,condition):
    checks.append(label)
    add("DO $$ BEGIN IF NOT COALESCE(("+condition+"),false) THEN RAISE EXCEPTION 'FAILED: "+label+"'; END IF; RAISE NOTICE 'PASS: "+label+"'; END $$;")
def seed(t='22',h='50',age='0',water='false',soil='50',pump='false',light='50',sample='1000'):
    add('TRUNCATE readings;')
    add(f"INSERT INTO readings(device_id,sampled_at_ms,temperature_c,humidity_pct,water_low,light_pct,moisture_pct,pump_on,is_test,received_at) VALUES ('01',{sample},{t},{h},{water},{light},{soil},{pump},false,now()-interval '{age} seconds');")
def alert(name,status):
    return f"SELECT \"Status\" LIKE '{status}%' FROM ({query(14)}) a WHERE \"Check\"='{name}'"
add('BEGIN;') # Stable now() throughout all boundary tests.
add('CREATE TABLE readings(id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,device_id text,sampled_at_ms bigint,temperature_c numeric,humidity_pct numeric,water_low boolean,light_pct numeric,moisture_pct numeric,pump_on boolean,is_test boolean,received_at timestamptz DEFAULT now());')
add('CREATE TABLE watering_events(id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,device_id text,boot_id text,cycle_id bigint,event text,occurred_at_ms bigint,duration_ms bigint,reason text,is_test boolean,received_at timestamptz DEFAULT now(),UNIQUE(device_id,boot_id,cycle_id,event));')
# Check the backend's actual captured INSERT SQL against explicit fixture schema.
fixtures=json.loads((HERE/'backend-query-fixtures.json').read_text())
def literal(v):
    if v is None:return 'NULL'
    if isinstance(v,bool):return 'true' if v else 'false'
    if isinstance(v,(int,float)):return str(v)
    return "'"+v.replace("'","''")+"'"
for f in fixtures:
    add(re.sub(r'\$(\d+)',lambda m:literal(f['values'][int(m[1])-1]),f['sql'])+';')
check('backend INSERT fixtures store four readings','SELECT count(*)=4 FROM readings')
check('backend ON CONFLICT keeps one start and one stop for a repeated cycle','SELECT count(*)=2 FROM watering_events')
add('TRUNCATE readings,watering_events;')
# Three cycles with matching timestamps ensure secondary ordering is exercised.
for mode,base in [(False,22),(True,24)]:
    for i in range(3):
        add(f"INSERT INTO readings(device_id,sampled_at_ms,temperature_c,humidity_pct,water_low,light_pct,moisture_pct,pump_on,is_test,received_at) VALUES ('01',{i},{base+i},50,false,50,50,false,{str(mode).lower()},now());")
        for ev,at,dur,reason in [('started',1000,'NULL','dry_soil'),('stopped',3000,'2000','time_limit')]:
            add(f"INSERT INTO watering_events(device_id,boot_id,cycle_id,event,occurred_at_ms,duration_ms,reason,is_test) VALUES ('01','{'a' if mode else 'b'}234567890abcdef',{i+1},'{ev}',{at},{dur},'{reason}',{str(mode).lower()});")
for mode,expected in [('Real',24),('Test',26)]:
    for pid in panels:
        count=6 if pid in (13,14) else 3 if pid in (9,10,11,12) else 1
        check(f'panel {pid} executes with expected row count in {mode}',f'SELECT count(*)={count} FROM ({query(pid,mode)}) q')
    check(f'latest value and tie break in {mode}',f'SELECT "Temperature"={expected} FROM ({query(1,mode)}) q')
    check(f'history values stay in {mode}',f'SELECT min("Temperature")={expected-2} AND max("Temperature")={expected} FROM ({query(9,mode)}) q')
    check(f'watering start stop order in {mode}',f"SELECT array_agg(\"Action\")=ARRAY['Started','Stopped','Started','Stopped','Started','Stopped'] AND count(*) FILTER (WHERE \"Duration\"=2)=3 FROM ({query(13,mode)}) q")
for t,h in [('16','40'),('27','60')]:
    seed(t,h);check(f'normal inclusive boundaries {t}C {h}pct',f"SELECT bool_and(\"Status\"='OK') FROM ({query(14)}) q")
for t,h,label in [('15.9','39.9','LOW'),('27.1','60.1','HIGH')]:
    seed(t,h);check(label+' temperature warning',alert('Temperature',label));check(label+' humidity warning',alert('Humidity',label))
seed(water='true');check('low reservoir warning',alert('Reservoir','LOW WATER'))
seed(t='NULL',h='NULL',water='NULL',soil='NULL',pump='NULL')
for name in ['Temperature','Humidity','Reservoir','Soil sensor','Pump status']:check('missing '+name+' fault',alert(name,'FAULT'))
seed(age='30');check('exactly 30 seconds is fresh',alert('Incoming data','OK'))
seed(age='30.001');check('older than 30 seconds is stale',alert('Incoming data','STALE'));check('stale sensor state unknown',alert('Temperature','UNKNOWN'))
add('TRUNCATE readings;');check('no data state',alert('Incoming data','NO DATA'))
seed(sample='1');check('fresh receipt of old uptime sample marked OK',alert('Incoming data','OK'))
for light in ['0','100','NULL']:
    seed(light=light);check('no light warning even with light '+light,f"SELECT count(*)=6 AND bool_and(\"Status\"='OK') FROM ({query(14)}) q")
add('ROLLBACK;')
(HERE/'dashboard-fixture-tests.sql').write_text('\n'.join(statements)+'\n')
with tempfile.TemporaryDirectory(prefix='greenhouse-sql-') as directory:
    data=Path(directory)/'cluster'
    init=subprocess.run(['initdb','-D',str(data),'--no-locale','--encoding=UTF8','--auth=trust'],capture_output=True,text=True,check=True)
    result=subprocess.run(['postgres','--single','-D',str(data),'-c','log_min_messages=notice','postgres'],input='\n'.join(statements)+'\n',capture_output=True,text=True)
    raw=result.stdout+'\n'+result.stderr
    (HERE/'postgres-execution.log').write_text(raw)
    if result.returncode or re.search(r'\b(ERROR|FATAL|PANIC):',raw):raise RuntimeError(raw[-12000:])
    observed=re.findall(r'NOTICE:\s+PASS: ([^\n]+)',raw)
    assert len(observed)==len(checks),(len(observed),len(checks))
lines=['Dashboard SQL execution — 26 October 2026','Source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest(),f'PASS: {len(checks)} SQL assertions; all 14 panels executed in Real and Test modes.','PASS: inclusive air thresholds, low-water and null warnings, 30-second freshness boundary, latest tie break, mode separation and three-cycle history order.','GAP: fresh database receipt marks a delayed/old-uptime payload OK; this does not establish acquisition freshness.','GAP: no light alert for values 0, 100 or NULL.','Scope: actual query text, with Grafana mode/time macro substitutions, ran on disposable PostgreSQL fixture tables. The assumed UNIQUE key allowed duplicate-event suppression. This is not evidence of the deployed schema, live Grafana display, end-to-end latency or backend restart persistence.']
(HERE/'dashboard-results.txt').write_text('\n'.join(lines)+'\n');print('\n'.join(lines))
