import subprocess
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field
from typing import List, Optional
import uvicorn
import json
from multiprocessing import Process

app = FastAPI()


with open('test/testData.json', 'r') as file:
    global jsonStore
    jsonStore= json.load(file)


class Item(BaseModel):
    name: str

# Define endpoints
@app.get("/")
async def read_root():
    return {"Available": "1"}


class RequestModel(BaseModel):
    resolveRoots: bool = Field(
        False,
        alias="resolveRoots"
    )
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
    if (projectName == "TestPrjName"):
        return{"work": "/home/workh/Documents/AyonAos"} 


def start():
    proc = Process(target=uvicorn.run,args=(app,),kwargs={"host": "0.0.0.0","port": 8003,"log_level": "error"})

    proc.start()
    print("Server is starting in the background...")
    return proc


if __name__ == "__main__":
    test = start()
    del test 
    import requests
    response = requests.get("http://localhost:8003/")
    print("Test Respone", response.text)

