import sys
import re

args = sys.argv[1:]

def name2sane(name):
        match name.split("_")[0].strip().lower():
                case "mux":
                        return f"mout_{('_'.join(name.split('_')[1:]).strip().lower())}"
                case "pll":
                        return f"fout_{name.split('_')[1].lower()}_pll"
                case "oscclk":
                        return "oscclk"
                case "div":
                        if name.split("_")[1].strip() == "PLL":
                                return f"dout_{(''.join(name.split('_')[2]).lower())}_{(''.join(name.split('_')[3]).lower())}"
                        else:
                                return f"dout_{'_'.join((name.split('_')[1:]))}"
                case "gate" | "gout":
                        return f"gout_{'_'.join(name.split('_')[1:])}"
                case "":
                        return None
                case _:
                        print("error:")
                        print(name.split("_")[0], name)
                        return None


o = open("input-clk", "r")
lines = o.readlines()

print ("Step one: find the parents")

clk = args[0]

index = 0

parents = []

for line in lines:
        if args[0].lower()+"_parents" in line and "enum" in line:
                index = lines.index(line)

if index == 0:
        for line in lines:
                if re.search("CLK_DIV\("+clk.upper()+",", line, re.IGNORECASE) is not None:
                        parents.append(line.split(",")[1][1:]) # [1:] to get rid of space
                        break
                if re.search("CLK_GATE\("+clk.upper()+",", line, re.IGNORECASE) is not None:
                        parents.append(line.split(",")[1][1:]) # [1:] to get rid of space
                        break

if parents == []: # Parents not found yet, a mux?
        print ("Found parents structure at line", index)

        maxlines = 10
        endindex = 0

        while maxlines != 0:
                if "}" in lines[index + (10 - maxlines)]:
                        endindex = index + (10 - maxlines)
                        break
                maxlines -= 1

        if maxlines == 0:
                print("Too many parents?")
                exit(0)

        targetparents = "".join(lines[index:endindex])
        targetparents = targetparents.split("{")[1]

        for parent in targetparents.split(","):
                name = name2sane(parent)
                if name is None:
                        continue
                parents.append(name2sane(parent))

parents_2 = [] # There must be a way to do it in just one function call...

for parent in parents:
        parents_2.append(f'"{parent}",') 

parents_2[-1] = parents_2[-1].replace(",", "") # We don't need the comma in the last entry

print("Step 2: generate string")

match clk.split("_")[0].lower():
        case "mux":
                print(f"PNAME({name2sane(clk)+'_p'}) = {{ {' '.join(parents_2)} }};")

                mux_sel = ""

                for line in lines:
                        if clk.upper() in line and ("MUX_SEL" in line or "SELECT" in line):
                                mux_sel = ",".join(line.split(",")[1:3])
                                break

                mux_sel = mux_sel[1:] # get rid of space

                print(f'MUX({name2sane(clk).upper()}, "{name2sane(clk).lower()}", {name2sane(clk).lower()+"_p"}, {"CLK_CON_"+clk.upper()}, {mux_sel}),')
        case "div":
                div_ratio = ""
                for line in lines:
                        if re.search("SFR_ACCESS\(CLK_CON_DIV_"+clk.upper()+"_DIVRATIO", line) is not None:
                                div_ratio = ",".join(line.split(",")[1:3])[1:]
                print(f'DIV({name2sane(clk).upper()}, "{name2sane(clk).lower()}", "{name2sane(parents[0]).lower()}", {"CLK_CON_"+clk.upper()}, {div_ratio}),')
        case "gate" | "gout":
                print(f'GATE({name2sane(clk).upper()}, "{name2sane(clk).lower()}", "{name2sane(parents[0]).lower()}", {"CLK_CON_"+clk.upper()}, 21, CLK_IGNORE_UNUSED, 0),')
	# /* APM */
	# GATE(GOUT_CLKCMU_APM_BUS, "gout_clkcmu_apm_bus", "mout_clkcmu_apm_bus",
	#      CLK_CON_GATE_CLKCMU_APM_BUS, 21, CLK_IGNORE_UNUSED, 0),