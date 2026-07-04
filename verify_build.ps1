if (Test-Path "build\LiveExplorer_Skin.exe") {
    Write-Host "Build Successful: build\LiveExplorer_Skin.exe exists."
} else {
    Write-Host "Build Failed: build\LiveExplorer_Skin.exe not found."
}
if (Test-Path "build\image\source.png") {
    Write-Host "Image Copied: Yes"
} else {
    Write-Host "Image Copied: No"
}