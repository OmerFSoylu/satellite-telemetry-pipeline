import stomp_client, time, random

def tm_producer():
    mnemonics = [
        "PRESSURE", "TEMPERATURE", "VOLTAGE", "CURRENT", 
        "ALTITUDE", "SPEED", "FUEL_LEVEL", "ENGINE_RPM"
    ]
  
    mnemonic = random.choice(mnemonics)
    value = round(random.uniform(0, 100), 2)

    return mnemonic, value



def tm_listener(client, mnemonic, value):
    client.send(mnemonic, value)
    



if __name__ == "__main__":
    client = stomp_client.StompClient()
    client.connect()
    try:
        while True:
            mnemonic, value = tm_producer()
            tm_listener(client, mnemonic, value)
            time.sleep(1)
    except KeyboardInterrupt:
        client.disconnect()
