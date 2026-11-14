-- AlphaSmart NEO (MAME 0.289) boot PC probe.
-- Original, non-invasive; works with asneob/asneow in the single neo-diag.
-- Invoke with -autoboot_script scripts/neo_boot_pc_probe.lua
local m=manager.machine
local cpu=m.devices[":maincpu"]
if not cpu then print("NEO_PROBE_FATAL no :maincpu"); return end
local st=cpu.state
local mem=cpu.spaces["program"]
local frames,sys,app,distinct_count,last=0,0,0,0,nil
local pcs={}
local function val(k) local e=st[k];return e and e.value or nil end
local function hx(v) return type(v)=="number" and string.format("%08X",v & 0xffffffff) or "NA" end
local function sample()
    local pc=val("PC") or val("CURPC")
    frames=frames+1
    if pc then
        if not pcs[pc] then pcs[pc]=true;distinct_count=distinct_count+1 end
        if pc>=0x00410000 and pc<0x00470000 then sys=sys+1 end
        if pc>=0x00470000 and pc<0x00490000 then app=app+1 end
    end
    last=pc
    if frames<=20 or frames%60==0 then
        print(string.format("NEO_FRAME %d PC=%s CURPC=%s SP=%s SR=%s A0=%s D0=%s",frames,
          hx(pc),hx(val("CURPC")),hx(val("SP")),hx(val("SR")),hx(val("A0")),hx(val("D0"))))
    end
end
print("NEO_PROBE_BEGIN "..tostring(m.system.name))
if mem then
  print(string.format("NEO_VECTORS %s %s flash=%s %s os_entry=%s",
     hx(mem:read_u32(0)),hx(mem:read_u32(4)),hx(mem:read_u32(0x00400000)),
     hx(mem:read_u32(0x00400004)),hx(mem:read_u16(0x00410082))))
end
sample()
NEO_FRAME_SUB=emu.add_machine_frame_notifier(sample)
NEO_STOP_SUB=emu.add_machine_stop_notifier(function()
 print(string.format("NEO_PROBE_END frames=%d distinct=%d system_samples=%d applet_samples=%d last_pc=%s",
     frames,distinct_count,sys,app,hx(last)))
end)
