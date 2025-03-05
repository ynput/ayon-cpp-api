import threading
import time
import uvicorn
import requests
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List, Optional
import json

app = FastAPI()
stop_event = threading.Event()  # Stop flag for graceful shutdown

# Load JSON data
with open('test/testData.json', 'r') as file:
    jsonStore = json.load(file)


class Item(BaseModel):
    name: str


@app.get("/")
async def read_root():
    return {"Available": "1"}


class RequestModel(BaseModel):
    resolveRoots: bool = Field(False, alias="resolveRoots")
    uris: List[str]


class ResolvedEntityModel(BaseModel):
    projectName: str
    folderId: str
    productId: str
    versionId: str
    representationId: str
    filePath: str
    target: Optional[str]


class ResolvedURIModel(BaseModel):
    uri: str
    entities: List[ResolvedEntityModel]


@app.post("/api/resolve")
async def resolve_uris(request: RequestModel) -> List[ResolvedURIModel]:
    resolved_uris = []
    for uri in request.uris:
        resolved_entity = jsonStore["Resolve"][uri]["Server"][-1]["entities"][0]
        resolved_uri = ResolvedURIModel(uri=uri, entities=[resolved_entity])
        resolved_uris.append(resolved_uri)
    return resolved_uris


@app.get("/api/projects/{projectName}/siteRoots")
async def SiteRoots(projectName: str):
    if projectName == "TestPrjName":
        return {"work": "/home/workh/Documents/AyonAos"}


# Function to run the server with graceful shutdown
def run_server():
    config = uvicorn.Config(app, host="0.0.0.0", port=8003, log_level="error")
    server = uvicorn.Server(config)

    while not stop_event.is_set():
        server.run()
        time.sleep(1)

    print("🚀 Server shutting down...")


def start():
    server_thread = threading.Thread(target=run_server, daemon=True)
    server_thread.start()
    print("✅ Server is starting in the background...")
    return server_thread


def stop():
    print("🛑 Stopping server...")
    stop_event.set()  # Set the stop flag
    time.sleep(1)  # Give some time for shutdown


if __name__ == "__main__":
    try:
        server = start()

        # Wait until the server is responsive before sending a request
        for _ in range(10):  # Try for 10 seconds
            try:
                response = requests.get("http://localhost:8003/")
                if response.status_code == 200:
                    print("✅ Server is running:", response.text)
                    break
            except requests.exceptions.ConnectionError:
                print("⏳ Waiting for server to start...")
                time.sleep(1)
        else:
            print("❌ Error: Server did not start in time.")

        # Keep the script running
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n🛑 KeyboardInterrupt detected. Shutting down...")
        stop()
