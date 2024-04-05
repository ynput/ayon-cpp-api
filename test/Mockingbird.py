
import subprocess
from fastapi import FastAPI

# Create FastAPI app
app = FastAPI()

# Define endpoints
@app.get("/")
async def read_root():
    return {"message": "Hello, World"}

@app.get("/items/{item_id}")
async def read_item(item_id: int, q: str = None):
    return {"item_id": item_id, "q": q}

@app.post("/shutdown")
async def shutdown_server():
    subprocess.run(["pkill", "-f", "uvicorn"])  # Stop the uvicorn process
    return {"message": "Shutting down server..."}

