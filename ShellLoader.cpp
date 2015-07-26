/*
 * Copyright (c) 2015, Igalia S.L.
 * Copyright (c) 2015, Metrological
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ShellLoader.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

namespace Athol {

void ShellLoader::load(API::Compositor* compositor)
{
    const char* shellPath = getenv("ATHOL_SHELL");
    if (!shellPath)
        return;

    fprintf(stderr, "[Athol] Loading shell %s\n", shellPath);
    void* shell = dlopen(shellPath, RTLD_NOW);
    if (!shell) {
        fprintf(stderr, "[Athol] dlopen() failed: %s\n", dlerror());
        return;
    }

    fprintf(stderr, "[Athol] dlopen() successful\n");
    auto function = reinterpret_cast<ModuleInit>(dlsym(shell, "module_init"));
    if (!function)
        return;

    std::fprintf(stderr, "[Athol] module_init %p\n", function);
    function(compositor);
}

} // namespace Athol
