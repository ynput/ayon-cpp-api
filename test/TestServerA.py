import TestServer


if __name__ == "__main__":
    test = TestServer.start()
    del test 
    import requests
    response = requests.get("http://localhost:8003/")
    print("Test Respone", response.text)

