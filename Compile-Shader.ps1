<#
.SYNOPSIS
    Compiles a single .hlsl file into its composite .cso compiled shader files.
.DESCRIPTION
    Compile a single .hlsl file into its composite .cso compiled shader files. Certain pairs of
    switches are required when any of them are used. At a minimum, a shader must
    use -vs and -ps to be a valid renderable shader. To correctly use the
    Tessellation stage both -hs and -ds must be used together. The geometry stage
    is optional in all use cases and can be used by itself. The compute shader
    stage is also optional and can be used by itself.
.PARAMETER Path
    The path to the .hlsl script to compile.
.PARAMETER vs
    Tells the script to compile as a Vertex Shader.
    The entry point of the shader must be VertexFunction.
    If used, -ps must also be supplied.
.PARAMETER ps
    Tells the script to compile as a Pixel Shader.
    The entry point of the shader must be PixelFunction.
    If used, -vs must also be supplied.
.PARAMETER hs
    Tells the script to compile as a Hull Shader.
    The entry point of the shader must be HullFunction.
    If used, -ds must also be supplied.
.PARAMETER ds
    Tells the script to compile as a Domain Shader.
    The entry point of the shader must be DomainFunction.
    If used, -hs must also be supplied.
.PARAMETER gs
    Tells the script to compile as a Geometry Shader.
    The entry point of the shader must be GeometryFunction.
    This switch is optional.
.PARAMETER cs
    Tells the script to compile as a Compute Shader.
    The entry point of the shader must be ComputeFunction.
    This switch it optional and can appear by itself.
.PARAMETER d
    Tells the script to compile in debug mode.
    This enables validation and disables optimizations.
.INPUTS
    None
.OUTPUTS
    None
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "renderableshader.hlsl" -vs -ps
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "tesselatedshader.hlsl" -hs -ds
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "computeshader.hlsl" -cs
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "geometryshader.hlsl" -gs
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "renderabletesselatedshader.hlsl" -vs -ps -hs -ds
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "renderablegeometryshader.hlsl" -vs -ps -gs
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "renderablecomputeshader.hlsl" -vs -ps -cs
.EXAMPLE
    PS> Compile-Shader.ps1 -Path "complexshader.hlsl" -vs -ps -hs -ds -gs -cs

#>
[CmdletBinding()]
param(
    [Parameter( Mandatory )]
    [ValidateNotNullOrEmpty()]
    [System.IO.FileInfo]$Path,
    [Parameter( Mandatory , ParameterSetName = 'VSPS')]
    [Switch]$vs,
    [Parameter( Mandatory , ParameterSetName = 'VSPS' )]
    [Switch]$ps,
    [Parameter( Mandatory , ParameterSetName = 'HSDS' )]
    [Parameter( ParameterSetName = 'VSPS')]
    [Switch]$hs,
    [Parameter( Mandatory , ParameterSetName = 'HSDS' )]
    [Parameter( ParameterSetName = 'VSPS')]
    [Switch]$ds,
    [Parameter( Mandatory , ParameterSetName = 'GS' )]
    [Parameter( ParameterSetName = 'VSPS')]
    [Parameter( ParameterSetName = 'HSDS')]
    [Switch]$gs,
    [Parameter( Mandatory , ParameterSetName = 'CS' )]
    [Parameter( ParameterSetName = 'VSPS')]
    [Parameter( ParameterSetName = 'HSDS')]
    [Switch]$cs,
    [Switch]$d
)

function Test-Args {
    [CmdletBinding()]
    [OutputType([bool])]
    param(
        [Parameter(Mandatory=$false)]
        [bool]$vs,
        [Parameter(Mandatory=$false)]
        [bool]$hs,
        [Parameter(Mandatory=$false)]
        [bool]$ds,
        [Parameter(Mandatory=$false)]
        [bool]$gs,
        [Parameter(Mandatory=$false)]
        [bool]$ps,
        [Parameter(Mandatory=$false)]
        [bool]$cs
    )
    $nostages = $false -eq ($vs -or $hs -or $ds -or $gs -or $ps -or $cs)
    if(!$nostages) {
        $vsps = !(!$vs -or !$ps) -and !($vs -xor $ps)
        $hsds = !(!$hs -or !$ds) -and !($hs -xor $ds)
        $invalid = -not ($vsps -or $hsds -or $gs -or $cs)
        if($invalid) {
            Write-Output "Missing required arguments. VS and PS, HS and DS, or GS, or CS"
            Return $false
        }
        Return $true
    } else {
        Write-Output "No shader stages specified."
        Return $false
    }
}

function Get-EntryPointForStage {
    [CmdletBinding()]
    [OutputType([String])]
    param(
        [Parameter(Mandatory)]
        [string]$stageId
    )
    if($stageId -eq "vs") { Return "VertexFunction" }
    if($stageId -eq "ps") { Return "PixelFunction" }
    if($stageId -eq "hs") { Return "HullFunction" }
    if($stageId -eq "ds") { Return "DomainFunction" }
    if($stageId -eq "gs") { Return "GeometryFunction" }
    if($stageId -eq "cs") { Return "ComputeFunction" }
    Return $null
}

function Get-StagesFromArgs() {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]
        [bool]$vs,
        [Parameter(Mandatory)]
        [bool]$hs,
        [Parameter(Mandatory)]
        [bool]$ds,
        [Parameter(Mandatory)]
        [bool]$gs,
        [Parameter(Mandatory)]
        [bool]$ps,
        [Parameter(Mandatory)]
        [bool]$cs
    )
    $array = @()
    if($vs) { $array += "vs" }
    if($hs) { $array += "hs" }
    if($ds) { $array += "ds" }
    if($gs) { $array += "gs" }
    if($ps) { $array += "ps" }
    if($cs) { $array += "cs" }
    return $array
}

$validPath = Test-Path -Path $Path
if(!$validPath) {
    Write-Output "Script: $Path does not exist."
    Return;
}

$validArgs = Test-Args -vs $vs -hs $hs -ds $ds -gs $gs -ps $ps -cs $cs
if($validArgs) {
    $file = (Get-Item $Path).BaseName #$Path.Substring(0, $Path.LastIndexOf('.'))
    $ShaderArgs = Get-StagesFromArgs -vs $vs -hs $hs -ds $ds -gs $gs -ps $ps -cs $cs
    if($d) {
        Write-Output "Debug mode."
    } else {
        Write-Output "Release mode."
    }
    foreach($stage in $ShaderArgs) {
        $CsoPath = $file + "_" + $stage.ToUpper() + ".cso"
        $EntryPoint = Get-EntryPointForStage $stage
        $target = $stage + "_5_0"
        if($d) {
            fxc.exe /T $target /Fo $CsoPath /E $EntryPoint /Zi /Od /WX /nologo /Zpc $Path
        } else {
            fxc.exe /T $target /Fo $CsoPath /E $EntryPoint /O3 /WX /Vd /nologo /Zpc $Path
        }
    }
} else {
    Write-Output "Script terminated due to invalid arguments."
}
