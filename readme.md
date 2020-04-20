# Learning DirectX 12

## References
- https://www.3dgep.com/category/graphics-programming/directx/directx-12/
- http://alextardif.com/D3D11To12P1.html
- http://alextardif.com/RenderingAbstractionLayers.html
- https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide

## Executables
- L1.Basic_Window
- L2.Draw_Cube

## Basic Program Flow
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
|
set window callbacks
|-> on keypress
|   |-> stop drawing
|-> on mousemove
|   |-> *nothing*
|
show window
|
loop while window exists and continue to draw
|-> process window messages
|-> clock tick
|-> update frame
|-> draw frame
|   |-> transition buffer to render target
|   |-> wait for gpu to signal completed execution of previous frame
|   |-> open command list
|   |-> clear render target
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