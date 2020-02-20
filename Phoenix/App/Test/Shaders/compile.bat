%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1raygen.rgen -o %1SpirV/raygen.rgen.spv
%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1miss.rmiss -o %1SpirV/miss.rmiss.spv
%VULKAN_SDK%\Bin32\glslangValidator.exe -V %1closesthit.rchit -o %1SpirV/closesthit.rchit.spv
pause