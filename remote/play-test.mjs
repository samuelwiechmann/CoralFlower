import fs from 'node:fs';
import assert from 'node:assert/strict';
const mod = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
function play(target, lean, mix) {
    let ms = 0, rendered = 0;
    const wasi = {clock_time_get: (_clock, _precision, ptr) => { d.setBigUint64(ptr, process.hrtime.bigint(), true); return 0; }, fd_write: (_fd, iov, n, out) => { let count = 0; for(let i=0;i<n;i++) count += d.getUint32(iov+8*i+4,true); d.setUint32(out,count,true); return 0; }, fd_read: () => 8, fd_close: () => 8, fd_seek: () => 8,
                  proc_exit: code => { throw Error('exit ' + code); }};
    const e = new WebAssembly.Instance(mod, {wasi_snapshot_preview1:wasi}).exports;
    e._initialize?.();
    const d = new DataView(e.memory.buffer), descriptor = e.rfm1_get(6);
    assert(descriptor);
    const panel = d.getUint32(descriptor + e.rfm1_layout(30), true);
    const field = i => panel + e.rfm1_layout(i);
    d.setUint8(field(16), 2);
    e.rfm1_options(0, 1, 0, 1);
    e.rfm1_init(0);
    const buf = e.rfm1_alloc(512), packet = e.rfm1_alloc(4);

    function step(n, samples) {
        for (let end = ms+n; ms < end; ms++) {
            e.rfm1_tick(ms);
            if (ms % 16 === 0) e.rfm1_frame();
            else e.rfm1_idle();
            while (rendered+128 <= ms*44.1) {
                e.rfm1_audio(buf, 128);
                rendered += 128;
                if (samples) samples.push(...new Int16Array(e.memory.buffer,buf,256));
            }
        }
    }
    function button(role) {
        d.setUint32(field(2), 1 << d.getUint8(field(13)+role), true); step(64);
        d.setUint32(field(2), 0, true); step(64);
    }
    function turn(role, amount) {
        d.setInt32(field(5) + 4*d.getUint8(field(14)+role), amount*d.getInt8(field(15)+role), true);
        step(48);
    }
    step(2600);
    button(0); // FX. SELECT advances through the saved chain.
    const slot = [0,0,0,1,3][target];
    for (let i = 0; i < slot; i++) turn(0, 1);
    turn(3, 1);
    if (!slot) for (let i = 0; i < 50; ++i) turn(5,-1);
    button(5); turn(0,-1); turn(3,1); turn(5,1);
    for (let i = 0; i < target; i++) turn(6,1);
    if (mix) {
        turn(6,5); // select Multi
        turn(0,-1); // Expression -> AT Mix; K1 browses, K2 edits amount.
        for (const [role, steps] of mix) {
            if (steps) turn(role,steps); else step(48);
        }
    }
    button(6);
    d.setUint8(field(7)+7, 85);
    d.setUint32(field(1), 1<<7, true);
    step(80);
    const strikes = [];
    while (e.rfm1_midi_out_take(packet)) {
        const value = d.getUint32(packet,true);
        if (((value>>>12)&15) === 9) strikes.push(value>>>24);
    }
    assert(strikes.includes(85), 'panel strike did not reach MIDI');
    d.setUint8(field(8)+7, lean);
    const samples = [];
    step(400,samples);
    assert(!e.rfm1_halted());
    assert(samples.some(v => Math.abs(v)>50));
    return samples;
}
// Identical time and input history: vary only the amount detents.
function compareMix(cutSteps, tremSteps) {
    return play(0,100,[
        ...Array.from({length:12},(_,i)=>[4,i<cutSteps?-1:0]),
        [3,1],
        ...Array.from({length:5},(_,i)=>[4,i<tremSteps?1:0]),
    ]);
}
for (const [i,name] of ['Cutoff','Tremolo','Freq shift','Drive','Phaser'].entries()) {
    const dry = play(i,0), wet = play(i,100);
    const rms = Math.sqrt(dry.reduce((s,v,j)=>s+(v-wet[j])**2,0)/dry.length);
    assert(rms > 1, name + ' pressure produced no audible sample difference');
    console.log(name, 'pressure difference RMS', rms.toFixed(1));
}



const layered = compareMix(12,5); // 40% cutoff + 25% tremolo
for (const single of [compareMix(12,0), compareMix(0,5)]) {
    assert.equal(single.length,layered.length);
    assert(layered.some((v,i)=>Math.abs(v-single[i])>50));
}
console.log('Layered cutoff and tremolo respond independently.');
