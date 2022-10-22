i = open ("cmu_clocks", "r")

data = i.readlines()
strings = []
regs = []

for line in data:
        line = line.strip()
        a = line.split("(")
        b = a[1].split(",")
        strings.append(f"#define {b[0]}  {b[1][1:]}")
strings.sort(key=lambda x: int(x.split()[2], 16))

for string in strings:
        regs.append(f"{string.split(' ')[1]},")

print("\n".join(strings))

print("")

print("\n".join(regs))


counter = 1

for string in strings:
        split = string.split()
        name = split[1]
        name_split = name.split("_")
        match "_".join(string.split()[1].split("_")[:3:]):
                case "PLL_LOCKTIME_PLL":
                        continue
                case "PLL_CON0_PLL":
                        print(f"#define FOUT_{name_split[3]}_PLL  {counter}")
                case "PLL_CON3_PLL":
                        continue
                case "CLK_CON_MUX":
                        print(f"#define MOUT_{'_'.join(name_split[3:])}  {counter}")
                case "CLK_CON_DIV":
                        print(f"#define DOUT_{'_'.join(name_split[3:])}  {counter}")
                case "CLK_CON_GATE":
                        print(f"#define GOUT_{'_'.join(name_split[3:])}  {counter}")
                case _:
                        print(string)
                        print("_".join(string.split()[1].split("_")[:3:]))
                        break
        counter += 1
