from pathlib import Path
import re
import matplotlib.pyplot as plt

ICON_DIR = Path("./vector_icons")


# ===================== 解析 =====================

def parse_icon_file(path: Path):
    raw = path.read_text(encoding="utf-8")

    lines = []
    for l in raw.splitlines():
        l = l.strip()
        if not l or l.startswith("//"):
            continue
        lines.append(l)

    tokens = [t for t in re.split(r"[,\s]+", "\n".join(lines)) if t]

    cmds = []
    i = 0

    while i < len(tokens):
        t = tokens[i]

        if t == "CANVAS_DIMENSIONS":
            cmds.append(("CANVAS", int(tokens[i + 1])))
            i += 2

        elif t == "PATH_COLOR_ARGB":
            cmds.append(("COLOR",
                int(tokens[i + 1], 16),
                int(tokens[i + 2], 16),
                int(tokens[i + 3], 16),
                int(tokens[i + 4], 16)
            ))
            i += 5

        elif t in ("MOVE_TO", "LINE_TO", "R_MOVE_TO", "R_LINE_TO"):
            cmds.append((t,
                float(tokens[i + 1].rstrip("f")),
                float(tokens[i + 2].rstrip("f"))
            ))
            i += 3

        elif t in ("H_LINE_TO", "V_LINE_TO", "R_H_LINE_TO", "R_V_LINE_TO"):
            cmds.append((t, float(tokens[i + 1].rstrip("f"))))
            i += 2

        elif t in ("CUBIC_TO", "R_CUBIC_TO"):
            vals = [float(tokens[i + j].rstrip("f")) for j in range(1, 7)]
            cmds.append((t, *vals))
            i += 7

        elif t in ("ARC_TO", "R_ARC_TO"):
            # 暂时跳过绘制，但保留游标移动
            vals = [
                float(tokens[i + 1].rstrip("f")),
                float(tokens[i + 2].rstrip("f")),
                float(tokens[i + 3].rstrip("f")),
                int(tokens[i + 4]),
                int(tokens[i + 5]),
                float(tokens[i + 6].rstrip("f")),
                float(tokens[i + 7].rstrip("f")),
            ]
            cmds.append((t, *vals))
            i += 8

        elif t in ("CLOSE", "NEW_PATH"):
            cmds.append((t,))
            i += 1

        else:
            raise RuntimeError(f"{path.name} 未知指令: {t}")

    return cmds


# ===================== 归一化（关键） =====================

def normalize(cmds):
    out = []

    cx = cy = 0.0
    sx = sy = 0.0

    for c in cmds:
        op = c[0]

        if op == "CANVAS" or op == "COLOR":
            out.append(c)

        elif op == "MOVE_TO":
            cx, cy = c[1], c[2]
            sx, sy = cx, cy
            out.append(("MOVE", cx, cy))

        elif op == "R_MOVE_TO":
            cx += c[1]
            cy += c[2]
            sx, sy = cx, cy
            out.append(("MOVE", cx, cy))

        elif op == "LINE_TO":
            cx, cy = c[1], c[2]
            out.append(("LINE", cx, cy))

        elif op == "R_LINE_TO":
            cx += c[1]
            cy += c[2]
            out.append(("LINE", cx, cy))

        elif op == "H_LINE_TO":
            cx = c[1]
            out.append(("LINE", cx, cy))

        elif op == "V_LINE_TO":
            cy = c[1]
            out.append(("LINE", cx, cy))

        elif op == "R_H_LINE_TO":
            cx += c[1]
            out.append(("LINE", cx, cy))

        elif op == "R_V_LINE_TO":
            cy += c[1]
            out.append(("LINE", cx, cy))

        elif op == "CUBIC_TO":
            out.append(("CUBIC", *c[1:]))
            cx, cy = c[5], c[6]

        elif op == "R_CUBIC_TO":
            out.append(("CUBIC",
                cx + c[1], cy + c[2],
                cx + c[3], cy + c[4],
                cx + c[5], cy + c[6]
            ))
            cx += c[5]
            cy += c[6]

        elif op in ("ARC_TO", "R_ARC_TO"):
            # 只移动当前点
            if op == "ARC_TO":
                cx, cy = c[6], c[7]
            else:
                cx += c[6]
                cy += c[7]

        elif op == "CLOSE":
            out.append(("CLOSE", sx, sy))
            cx, cy = sx, sy

        elif op == "NEW_PATH":
            pass

    return out


# ===================== 绘制 =====================

def draw_icon(cmds, title):
    fig, ax = plt.subplots()
    ax.set_aspect("equal")
    ax.invert_yaxis()
    ax.axis("off")
    ax.set_title(title)

    px = py = None

    for c in cmds:
        if c[0] == "CANVAS":
            s = c[1]
            ax.set_xlim(0, s)
            ax.set_ylim(0, s)

        elif c[0] == "MOVE":
            px, py = c[1], c[2]

        elif c[0] == "LINE":
            ax.plot([px, c[1]], [py, c[2]], color="black")
            px, py = c[1], c[2]

        elif c[0] == "CUBIC":
            ax.plot(
                [px, c[1], c[3], c[5]],
                [py, c[2], c[4], c[6]],
                color="black"
            )
            px, py = c[5], c[6]

        elif c[0] == "CLOSE":
            ax.plot([px, c[1]], [py, c[2]], color="black")
            px, py = c[1], c[2]

    plt.show(block=False)


# ===================== 主流程：逐张查看 =====================

def main():
    icons = sorted(ICON_DIR.glob("*.icon"))

    for i, p in enumerate(icons, 1):
        print(f"[{i}/{len(icons)}] {p.name}")
        raw = parse_icon_file(p)
        cmds = normalize(raw)
        draw_icon(cmds, p.stem)
        input("回车下一张...")
        plt.close("all")


if __name__ == "__main__":
    main()
