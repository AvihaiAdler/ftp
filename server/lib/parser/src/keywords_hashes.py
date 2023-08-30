from ctypes import c_size_t
import time
import math


class Result:
    def __init__(self, size: int, seed: int) -> None:
        self.size = size
        self.seed = seed

    def is_meaningful(self) -> bool:
        return self.size != 0


def hash(command: str, seed: int, size: int, /) -> int:
    shortened = command[:2] + command[-2:]

    _hash = c_size_t(0)
    for i in range(len(shortened)):
        _hash = c_size_t((_hash.value << (i * 8)) | ord(shortened[i]))
        _hash = c_size_t(seed * _hash.value)

    return _hash.value % size


def print_dict(d: dict) -> None:
    for k, v in d.items():
        print(f"({k}, {v})")


def find_minimal_size(commands: list[str]) -> Result:
    seeds = {}
    for seed in range(1, 10000):
        for size in range(len(commands), len(commands) * 10):
            hashes = {
                command.lower(): hash(command.lower(), seed, size)
                for command in commands
            }

            collisions = {}
            for _hash in hashes.values():
                collisions[_hash] = collisions.get(_hash, 0) + 1

            if max(collisions.values()) == 1:
                seeds[size] = seed

    if len(seeds) == 0:
        return Result(0, 0)

    min_size = min(seeds.keys())
    return Result(min_size, seeds[min_size])


def main() -> None:
    commands = [
        "USER",
        "PASS",
        "ACCT",
        "CWD",
        "CDUP",
        "SMNT",
        "REIN",
        "QUIT",
        "PORT",
        "PASV",
        "TYPE",
        "STRU",
        "MODE",
        "RETR",
        "STOR",
        "STOU",
        "APPE",
        "ALLO",
        "REST",
        "RNFR",
        "RNTO",
        "ABOR",
        "DELE",
        "RMD",
        "MKD",
        "PWD",
        "LIST",
        "NLST",
        "SITE",
        "SYST",
        "STAT",
        "HELP",
        "NOOP",
    ]

    res = find_minimal_size(commands)
    print(f"size: {res.size}, seed: {res.seed}")

    if res.is_meaningful():
        hashes = {
            command.lower(): hash(command.lower(), res.seed, res.size)
            for command in commands
        }
        print_dict(hashes)


if __name__ == "__main__":
    start = time.perf_counter()
    main()
    end = time.perf_counter()
    print(f"execution time: {((end-start)//60)}:{math.floor((end - start)%60)}")
