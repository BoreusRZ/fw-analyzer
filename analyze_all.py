import os
import asyncio
from datetime import datetime

log_file = open("log","w")
def log(msg):
    log_file.write(f"[{str(datetime.now())}] {msg}\n")
    log_file.flush()

async def shell(command):
    log(f"executing: {command}")
    logfile = f"logs/{command.replace(' ','_').replace('/','_').replace('&&','')}"
    # command = f"({command}) > {logfile} 2> {logfile}"
    command = f"({command})"
    #await asyncio.sleep(1)
    #return 0
    proc = await asyncio.create_subprocess_shell(command)
    return await proc.wait()

async def task(path,sem):
    async with sem:
        if os.path.isdir(f"firewall-config/"+path) and path != ".git":
            ruleset_arg = f"firewall-config/{path}/ruleset"
            ip_set_flag = f"firewall-config/{path}/ipset"
            if os.path.isfile(ip_set_flag):
                ip_set_flag = "--ipset " + ip_set_flag
            else:
                ip_set_flag = ""
            result_file = f"results/{path}_analysis"
            errlog_file = f"results/{path}_errlog"
            if os.path.isfile(result_file):
                return
            await shell(f"./build/release/analyzer {ruleset_arg} {ip_set_flag} -c -a consumers -V > {result_file} 2> {errlog_file}")

async def main():
    sem = asyncio.Semaphore(8)
    tasks = [task(path,sem) for path in os.listdir("firewall-config")]
    await asyncio.gather(*tasks)
asyncio.run(main())
