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
|-> create mesh buffer
|-> load shaders
|-> create root signature
|-> create pipeline state object
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
```