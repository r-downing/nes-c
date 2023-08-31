#include <nes_mapper.h>
#include <stddef.h>

// https://www.nesdev.org/wiki/Mapper

typedef struct {
    NesMapperInterface interface;
    void (*init)(NesMapper *);
    void (*deinit)(NesMapper *);
} MapperPrivateInterface;

///// mapper 0
static uint32_t mapper0_prg(struct NesMapper *const mapper, uint16_t addr) {
    return addr & ((((NesMapper *)mapper)->num_prg_banks > 1) ? 0x7FFF : 0x3FFF);
}

uint32_t mapper0_chr(struct NesMapper *, uint16_t addr) {
    return addr;
}

static const MapperPrivateInterface mapper0 = {
    .interface =
        {
            .prg_read = mapper0_prg,
            .prg_write = mapper0_prg,
            .chr_read = mapper0_chr,
            .chr_write = mapper0_chr,
        },
};

////// mapper 0

static const MapperPrivateInterface *const mapper_table[] = {
    &mapper0,
};

void nes_mapper_deinit(NesMapper *mapper) {
    for (size_t i = 0; i < (sizeof(mapper_table) / sizeof(mapper_table[0])); i++) {
        if (&mapper_table[i]->interface == mapper->interface) {
            if (NULL != mapper_table[i]->deinit) {
                mapper_table[i]->deinit(mapper);
            }
            mapper->interface = NULL;
            break;
        }
    }
}

void nes_mapper_init(NesMapper *const mapper, uint8_t num_prg_banks, uint8_t num_chr_banks, NesMapperType t) {
    mapper->num_chr_banks = num_chr_banks;
    mapper->num_prg_banks = num_prg_banks;
    const MapperPrivateInterface *const p = mapper_table[t];
    mapper->interface = &p->interface;
    if (NULL != p->init) {
        p->init(mapper);
    }
}