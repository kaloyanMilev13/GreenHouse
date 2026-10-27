// Execute the unchanged backend with MQTT/pg adapters. No broker or live DB writes.
const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert/strict'),crypto=require('crypto');
const root=path.resolve(__dirname,'../../../..');
const source=fs.readFileSync(path.join(root,'backend/index.js'),'utf8');
const handlers={},queries=[],errors=[],subscriptions=[];let dbFail=false;
const db={on(){},async query(sql,values){queries.push({sql,values});if(dbFail)throw Error('injected database outage');return {rows:[{id:1,test:1}]};}};
const client={on(name,cb){handlers[name]=cb;},subscribe(topics,opts,cb){subscriptions.push({topics,opts});cb(null);}};
const context=vm.createContext({require(name){if(name==='pg')return {Pool:function(){return db;}};if(name==='mqtt')return {connect(){return client;}};throw Error(name);},console:{log(){},error(...args){errors.push(args.join(' '));}},Buffer});
vm.runInContext(source,context,{filename:'backend/index.js'});
const telemetry={sampled_at_ms:1000,temperature_c:22,humidity_pct:50,water_low:false,light_pct:50,moisture_pct:0,pump_on:false,test:true};
const event={device_id:'01',boot_id:'1234567890abcdef',cycle_id:1,event:'started',occurred_at_ms:1000,duration_ms:null,reason:'dry_soil',test:true};
const topic='greenhouse/01/';let passed=0,validQueries=[];
async function send(kind,data){await handlers.message(topic+kind,Buffer.from(typeof data==='string'?data:JSON.stringify(data)));}
async function valid(kind,data){const n=queries.length,e=errors.length;await send(kind,data);assert.equal(queries.length,n+1);assert.equal(errors.length,e);validQueries.push(queries.at(-1));passed++;}
async function reject(kind,data){const n=queries.length,e=errors.length;await send(kind,data);assert.equal(queries.length,n);assert.equal(errors.length,e+1);passed++;}
(async()=>{
 await new Promise(r=>setImmediate(r));handlers.connect();assert.deepEqual(JSON.parse(JSON.stringify(subscriptions)),[{topics:[topic+'telemetry',topic+'watering'],opts:{qos:1}}]);passed++;
 await valid('telemetry',telemetry);assert.deepEqual(JSON.parse(JSON.stringify(queries.at(-1).values)),['01',1000,22,50,false,50,0,false,true]);
 await valid('telemetry',{...telemetry,temperature_c:null,humidity_pct:null,water_low:null,light_pct:null,moisture_pct:null,pump_on:null});
 await valid('telemetry',{...telemetry,temperature_c:-100,humidity_pct:0,light_pct:0,moisture_pct:100,test:false});
 await valid('telemetry',{...telemetry,temperature_c:150,humidity_pct:100,light_pct:100,moisture_pct:0});
 for(const change of [{sampled_at_ms:-1},{sampled_at_ms:1.1},{sampled_at_ms:Number.MAX_SAFE_INTEGER+1},{test:'true'},{temperature_c:'22'},{temperature_c:151},{humidity_pct:101},{light_pct:-1},{moisture_pct:101},{water_low:1},{pump_on:'false'},{humidity_pct:undefined}])await reject('telemetry',{...telemetry,...change});
 for(const data of ['{broken','null','[]','42'])await reject('telemetry',data);
 await valid('watering',event);
 for(const reason of ['time_limit','soil_wet','low_water','invalid_sensor'])await valid('watering',{...event,event:'stopped',occurred_at_ms:3000,duration_ms:2000,reason});
 for(const change of [{device_id:'02'},{boot_id:'bad'},{boot_id:'ABCDEF1234567890'},{cycle_id:0},{cycle_id:1.1},{occurred_at_ms:-1},{test:null},{event:'unknown'},{duration_ms:0},{reason:'soil_wet'}])await reject('watering',{...event,...change});
 for(const change of [{duration_ms:-1},{duration_ms:3001},{duration_ms:1.5},{duration_ms:null},{reason:'unknown'}])await reject('watering',{...event,event:'stopped',occurred_at_ms:3000,duration_ms:2000,reason:'time_limit',...change});
 let n=queries.length;await send('unrelated',telemetry);assert.equal(queries.length,n);passed++;
 dbFail=true;n=queries.length;let e=errors.length;await send('telemetry',telemetry);await new Promise(r=>setImmediate(r));assert.equal(queries.length,n+1);assert.equal(errors.length,e+1);dbFail=false;passed++;
 const result=[`Backend execution — 26 October 2026`,`Source SHA256 ${crypto.createHash('sha256').update(source).digest('hex')}`,`PASS: ${passed} cases including subscriptions, valid/null/boundary payloads, invalid messages, stop reasons and DB error handling.`,`PASS: valid payloads reached the parameterized INSERT statements with expected fields.`,`LIMIT: database failure logged after one insert attempt; no application retry observed.`,`Scope: actual backend code executed with MQTT and database adapters; delivery, commits, duplicate suppression and persistence are not established by this run.`];
 fs.writeFileSync(path.join(__dirname,'backend-results.txt'),result.join('\n')+'\n');fs.writeFileSync(path.join(__dirname,'backend-query-fixtures.json'),JSON.stringify(validQueries,null,2));console.log(result.join('\n'));
})().catch(e=>{console.error(e);process.exitCode=1;});
