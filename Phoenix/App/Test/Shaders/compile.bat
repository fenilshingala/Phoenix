%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1ref_raygen.rgen -o %1SpirV/ref_raygen.rgen.spv
%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1ref_miss.rmiss -o %1SpirV/ref_miss.rmiss.spv
%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1ref_closesthit.rchit -o %1SpirV/ref_closesthit.rchit.spv
pause