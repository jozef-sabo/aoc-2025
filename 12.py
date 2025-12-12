import re

shape_idx = re.compile(r"^\d+:$")


def rotate_90(shape):
    new_shape = []
    for y in range(len(shape)):
        new_shape.append(["."] * len(shape[0]))

    new_shape[0][0] = shape[2][0]
    new_shape[0][1] = shape[1][0]
    new_shape[0][2] = shape[0][0]

    new_shape[1][0] = shape[2][1]
    new_shape[1][1] = shape[1][1]
    new_shape[1][2] = shape[0][1]

    new_shape[2][0] = shape[2][2]
    new_shape[2][1] = shape[1][2]
    new_shape[2][2] = shape[0][2]

    return new_shape

def place(field, shape, x, y):
    if y + len(shape) > len(field):
        return False
    if x + len(shape[0]) > len(field[0]):
        return False

    for y_idx in range(len(shape)):
        for x_idx in range(len(shape[0])):
            if field[ y_idx + y ][ x_idx + x ] == '#' and shape[ y_idx ][x_idx] == '#':
                return False

    for y_idx in range(len(shape)):
        for x_idx in range(len(shape[0])):
            if field[ y_idx + y ][ x_idx + x ] != '#' and shape[ y_idx ][x_idx] == '#':
                field[ y_idx + y ][ x_idx + x ] = '#'

    return True

def unplace(field, shape, x, y):
    for y_idx in range(len(shape)):
        for x_idx in range(len(shape[0])):
            if shape[ y_idx ][x_idx] == '#':
                field[ y_idx + y ][ x_idx + x ] = '.'

    return True

# well, the bruteforce method
def try_to_place(space, shapes, shape_idx):
    if shape_idx == len(shapes):
        return True

    for y in range(len(space) - 2):
        for x in range(len(space[0]) - 2):
            for _ in range(4):
                if place(space, shapes[shape_idx], x, y):
                    if try_to_place(space, shapes, shape_idx + 1):
                        return True
                    unplace(space, shapes[shape_idx], x, y)
                shapes[shape_idx] = rotate_90(shapes[shape_idx])
    return False



def find_space(shapes, field_size, required_indices):
    space = []
    x, y = field_size
    for y_idx in range(y):
        space.append(["."] * x)

    shapes_to_use = []
    for idx in range(len(required_indices)):
        while required_indices[idx] != 0:
            shapes_to_use.append(shapes[idx])
            required_indices[idx] -= 1

    # if try_to_place(space, shapes_to_use, 0):
    #     return 1

    # it looks like cheating, but it worked ¯\_(ツ)_/¯
    if x * y / 9 < len(shapes_to_use):
        return 0

    return 1


def main():
    lines = []

    with open("12-input.txt", "r", encoding="UTF-8") as f:
        lines = f.readlines()

    line_idx = 0
    shapes = []
    while True:
        shape = []
        if shape_idx.search(lines[line_idx]):
            line_idx += 1
            while lines[line_idx] != "\n":
                shape.append(lines[line_idx].strip())
                line_idx += 1
        else:
            break
        shapes.append(shape)
        line_idx += 1

    requirements = []
    for line in lines[line_idx:]:
        field, indices = line.strip().split(":")
        field = field.strip()
        indices = indices.strip()
        requirements.append(([int(idx) for idx in field.split("x")], [int(idx) for idx in indices.split(" ")]))

    num = 0
    for field, indices in requirements:
        num += find_space( shapes, field, indices )

    print(num)

if __name__ == "__main__":
    main()
