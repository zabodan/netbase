set target=debug

erase /q logs\*.*

start "server" cmd /c "%target%\netbase_server.exe 130 > logs\_server.log"

for /L %%i in (1,1,10) do (
  start "client %%i" cmd /c "%target%\netbase_client.exe 100 > logs\_client_%%i.log"
)