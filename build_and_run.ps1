Write-Host "=== 开始构建流程 ==="

# 0.5 杀死正在运行的实例，解除文件锁定
Write-Host "检查并终止正在运行的程序实例..."
Get-Process -Name LiveExplorer_Skin -ErrorAction SilentlyContinue | Stop-Process -Force -Passthru | ForEach-Object {
    Write-Host "已终止运行中的程序: $($_.ProcessName)" -ForegroundColor Yellow
}
Start-Sleep -Seconds 1 # 等待文件释放

# 0. 设置环境变量 (针对 Qt 6.10.2 MinGW 环境)
# 自动检测并优先使用 Qt 自带的 MinGW 编译器，解决版本不兼容问题
if (Test-Path "E:\QT\Tools\mingw1310_64\bin") {
    Write-Host "检测到 Qt MinGW 工具链，正在设置环境..."
    $env:PATH = "E:\QT\Tools\mingw1310_64\bin;E:\QT\6.10.2\mingw_64\bin;$env:PATH"
} else {
    Write-Host "警告: 未找到 E:\QT\Tools\mingw1310_64，将尝试使用系统默认编译器..." -ForegroundColor Yellow
}

# 验证编译器版本
Write-Host "当前 g++ 版本:"
g++ --version | Select-Object -First 1

# 1. 清理旧的构建文件夹
$buildDir = "build"
if (Test-Path $buildDir) {
    Write-Host "正在清理旧的构建文件..."
    Remove-Item -Path $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $buildDir | Out-Null

# 2.5 手动编译资源文件 (绕过 cmake 路径中包含 '&' 导致的 windres 错误)
Write-Host "正在编译资源文件..."
$windres = "E:\QT\Tools\mingw1310_64\bin\windres.exe"
# 确保 gcc 在路径中
$env:PATH = "E:\QT\Tools\mingw1310_64\bin;$env:PATH"

# 将资源文件复制到 build 目录以避免复杂路径
Copy-Item "filemngr_bgimage.rc" "$buildDir\filemngr_bgimage.rc" -Force
if (Test-Path "icon.ico") {
    Copy-Item "icon.ico" "$buildDir\icon.ico" -Force
}

Push-Location $buildDir
# 尝试编译资源
$resSuccess = $false
if (Test-Path $windres) {
    try {
        & $windres -i "filemngr_bgimage.rc" -o "filemngr_bgimage_res.o"
        if ($LASTEXITCODE -eq 0) {
            $resSuccess = $true
            Write-Host "资源编译成功。" -ForegroundColor Green
        }
    } catch {
        Write-Host "资源编译异常: $_" -ForegroundColor Yellow
    }
}

if (-not $resSuccess) {
    Write-Host "资源编译失败，生成空对象文件以避免链接错误..." -ForegroundColor Yellow
    New-Item -ItemType File -Path "dummy_res.c" -Force | Out-Null
    gcc -c dummy_res.c -o filemngr_bgimage_res.o
}
Pop-Location

# 2. 切换到构建目录 (Wait, step 2.5 did Push/Pop, so we are back at root)
Push-Location $buildDir

# 3. 配置 CMake
Write-Host "正在配置 CMake..."
# 强制指定生成器为 MinGW Makefiles
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .. 

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake 配置失败。" -ForegroundColor Red
    Pop-Location
    exit
}

# 4. 编译
Write-Host "正在编译..."
mingw32-make -j8

if ($LASTEXITCODE -ne 0) {
    Write-Host "编译失败。" -ForegroundColor Red
    Pop-Location
    exit
}

# 4.5 部署与资源复制
# 注意：当前目录仍在 build 文件夹内
$exeFound = $false
$exeRelPath = ""

# 查找 exe 位置 (相对于 build 目录)
if (Test-Path "LiveExplorer_Skin.exe") { 
    $exeRelPath = "LiveExplorer_Skin.exe"
    $exeFound = $true
} elseif (Test-Path "Release\LiveExplorer_Skin.exe") { 
    $exeRelPath = "Release\LiveExplorer_Skin.exe" 
    $exeFound = $true
} elseif (Test-Path "Debug\LiveExplorer_Skin.exe") { 
    $exeRelPath = "Debug\LiveExplorer_Skin.exe" 
    $exeFound = $true
}

if ($exeFound) {
    $fullExePath = (Resolve-Path $exeRelPath).Path
    $exeDir = Split-Path $fullExePath
    Write-Host "找到可执行文件: $fullExePath" -ForegroundColor Cyan

    # 1. 复制图片资源
    # 源图片目录在上一级 (Code&Project/FileMngr_BGImage/image)
    $srcImageDir = Join-Path (Split-Path (Get-Location) -Parent) "image"
    if (Test-Path $srcImageDir) {
        $destImageDir = Join-Path $exeDir "image"
        Write-Host "正在复制图片资源从 $srcImageDir 到 $destImageDir ..."
        Copy-Item -Path $srcImageDir -Destination $exeDir -Recurse -Force
    } else {
        Write-Host "警告: 未找到源图片目录 $srcImageDir" -ForegroundColor Yellow
    }

    # 1.5 复制图标资源
    $projectRoot = Split-Path $srcImageDir -Parent
    $srcIcon = Join-Path $projectRoot "icon.ico"
    if (Test-Path $srcIcon) {
        $destIcon = Join-Path $exeDir "icon.ico"
        # 避免自我复制
        if ($srcIcon -ne $destIcon) {
            Write-Host "正在复制 icon.ico 至 $exeDir ..."
            Copy-Item -Path $srcIcon -Destination $destIcon -Force
        }
    }

    # 2. 部署 Qt 依赖
    $qtBin = "E:\QT\6.10.2\mingw_64\bin"
    $mingwBin = "E:\QT\Tools\mingw1310_64\bin"
    $windeployqt = Join-Path $qtBin "windeployqt.exe"

    if (Test-Path $windeployqt) {
        Write-Host "正在运行 windeployqt 部署 Qt依赖..."
        # 使用 --compiler-runtime 尝试自动部署 MinGW 库，如果不行则手动复制
        & $windeployqt --release --compiler-runtime --no-translations --force --dir $exeDir $fullExePath | Out-Host
    } else {
        Write-Host "错误: 未找到 windeployqt ($windeployqt)" -ForegroundColor Red
    }

    # 3. 确保 MinGW 运行时库存在 (libgcc, libstdc++, libwinpthread)
    Write-Host "检查并复制 MinGW 运行时库..."
    $mingwLibs = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
    foreach ($lib in $mingwLibs) {
        $srcLib = Join-Path $mingwBin $lib
        $destLib = Join-Path $exeDir $lib
        if (-not (Test-Path $destLib)) {
            if (Test-Path $srcLib) {
                Write-Host "复制 $lib ..."
                Copy-Item -Path $srcLib -Destination $destLib -Force
            } else {
                Write-Host "警告: 在 MinGW bin 中未找到 $srcLib" -ForegroundColor Yellow
            }
        }
    }

    # 5. 启动程序
    Write-Host "构建与部署完成，正在启动..." -ForegroundColor Green
    Start-Process $fullExePath
    Write-Host "程序已启动。请检查系统托盘。"
} else {
    Write-Host "错误: 未找到编译生成的 executable 文件。" -ForegroundColor Red
}

Pop-Location
