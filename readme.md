# Learning DirectX 12

## References
- https://www.3dgep.com/category/graphics-programming/directx/directx-12/
- http://alextardif.com/D3D11To12P1.html
- http://alextardif.com/D3D11To12P2.html
- http://alextardif.com/D3D11To12P3.html
- http://alextardif.com/RenderingAbstractionLayers.html
- https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide

## Executables
- L1.Basic_Window
- L2.Draw_Cube

## Program Flow
```flow
start 
|
create window
|
initialize directx 12 
|-> enable directx debug layer 
|-> init dx device
|-> init cmd queue, allocator & list -> close command list
|-> init gpu fences and event
|-> init dx swapchain
|-> init render target heap
|-> init render targets (back buffers) -> transition buffers to present
|-> init depth stencil heap
|-> init depth stencil buffer
|
initialize scene 
|-> create copy command queue
|-> get command list from copy-command-queue
|-> create vertex buffer
|   |-> create copy buffer (cpu side) *1*
|   |-> create vertex buffer (gpu side)
|   |-> create vertex buffer view
|-> create index buffer
|   |-> create copy buffer (cpu side) *1*
|   |-> create index buffer (gpu side)
|   |-> create index buffer view
|-> create root signature
|   |-> describe shader constants in root parameters
|   |-> identify signature flags
|   |-> create versioned-root-signature-description blob
|   |-> create root signature object
|-> create pipeline state object
|   |-> load vertex shader bytecode
|   |-> load pixel shader bytecode
|   |-> populate D3D12_SHADER_BYTECODE struct for vs and ps
|   |-> create input layout desc
|   |-> create graphics pipeline state desc
|   |   |-> set [root signature, input layout, vs, ps, 
|   |   |        rasterizer, sample, primitive topology, 
|   |   |        number of render targets, render target view format
|   |   |        depth stencil view format]
|   |-> create pipeline state object
|-> execute command list
|-> wait for execution to finish
|
set window callbacks
|-> on keypress
|   |-> escape -> stop-drawing
|-> on mousemove
|   |-> *nothing*
|
show window
|
loop while window-exists and not stop-drawing
|-> process window messages
|-> clock tick
|-> update frame
|-> draw frame
|   |-> transition buffer to render target
|   |-> wait for gpu to signal completed execution of previous frame
|   |-> open command list
|   |-> clear render target buffer
|   |-> clear depth stencil buffer
|   |
|   |-> *draw stuff using command list*
|   |
|   |-> transition buffer to present
|   |-> close command list
|   |-> execute command list
|   |-> give gpu a value to signal after execution of current frame
|   |-> present buffer
|   |-> select next frame buffer
|-> continue
|
report live dx objects
|
stop

*1*: object must remain alive till copy command list completes execution.
```
