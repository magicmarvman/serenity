/*
 * Copyright (c) 2020, Itamar S. <itamar8910@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/NonnullOwnPtrVector.h>
#include <AK/NonnullRefPtr.h>
#include <AK/Optional.h>
#include <AK/Vector.h>
#include <LibDebug/Dwarf/DwarfInfo.h>
#include <LibELF/Loader.h>
#include <Libraries/LibDebug/Dwarf/DIE.h>
#include <Libraries/LibDebug/Dwarf/LineProgram.h>
#include <sys/arch/i386/regs.h>

class DebugInfo {
public:
    explicit DebugInfo(NonnullRefPtr<const ELF::Loader> elf);

    struct SourcePosition {
        String file_path;
        size_t line_number { 0 };
        u32 address_of_first_statement { 0 };

        bool operator==(const SourcePosition& other) const { return file_path == other.file_path && line_number == other.line_number; }
        bool operator!=(const SourcePosition& other) const { return !(*this == other); }
    };

    struct VariableInfo {
        enum class LocationType {
            None,
            Address,
            Regsiter,
        };
        String name;
        String type;
        LocationType location_type { LocationType::None };
        union {
            u32 address;
        } location_data { 0 };

        NonnullOwnPtrVector<VariableInfo> members;
        VariableInfo* parent { nullptr };
    };

    struct VariablesScope {
        bool is_function { false };
        String name;
        u32 address_low { 0 };
        u32 address_high { 0 };
        Vector<Dwarf::DIE> dies_of_variables;
    };

    NonnullOwnPtrVector<VariableInfo> get_variables_in_current_scope(const PtraceRegisters&) const;

    Optional<SourcePosition> get_source_position(u32 address) const;
    Optional<u32> get_instruction_from_source(const String& file, size_t line) const;

    template<typename Callback>
    void for_each_source_position(Callback callback) const
    {
        String previous_file = "";
        size_t previous_line = 0;
        for (const auto& line_info : m_sorted_lines) {
            if (line_info.file == previous_file && line_info.line == previous_line)
                continue;
            previous_file = line_info.file;
            previous_line = line_info.line;
            callback({ line_info.file, line_info.line, line_info.address });
        }
    }

private:
    void prepare_variable_scopes();
    void prepare_lines();
    void parse_scopes_impl(const Dwarf::DIE& die);
    NonnullOwnPtr<VariableInfo> create_variable_info(const Dwarf::DIE& variable_die, const PtraceRegisters&) const;

    NonnullRefPtr<const ELF::Loader> m_elf;
    NonnullRefPtr<Dwarf::DwarfInfo> m_dwarf_info;

    Vector<VariablesScope> m_scopes;
    Vector<LineProgram::LineInfo> m_sorted_lines;
};
