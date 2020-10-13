Write-Host "Zastavuje sa UFOHD..."
Write-Host
Sleep 1
Stop-Process -processName "UFOHD*"
