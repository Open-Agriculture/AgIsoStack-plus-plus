# Environnement de mutation testing

## Versions de référence

Les campagnes de mutation testing de ce POC utilisent les versions suivantes :

- Système : Ubuntu 24.04
- Architecture : x86_64
- Clang : 19.1.1
- Clang++ : 19.1.1
- LLVM : 19.1.1
- Mull : 0.34.0

## Règle de reproductibilité

Ces versions sont figées pour toute la durée du POC.

Toutes les campagnes de mutation testing doivent utiliser Clang 19.1.1, LLVM 19.1.1 et Mull 0.34.0.

Si une de ces versions change, les nouveaux résultats ne doivent pas être comparés directement aux résultats précédents. Ce changement devra être documenté comme un nouvel environnement expérimental.
