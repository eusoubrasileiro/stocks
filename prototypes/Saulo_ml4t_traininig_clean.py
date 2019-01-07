svm = SVC(kernel='rbf', random_state=0, gamma=.01, C=3)

## CONFIGURAÇÃO DOS PARÂMETROS

janela = 180            # Dimensão da janela para cálculo de indicadores
desvio = 2              # Dimensão do desvio da janela dos indicadores
batch_size = janela        # Dimensão do Lote de memória para treinar o modelo ( features - X )
intervalo = 0.0005           # Intervalo entre as consultas de tickers no servidor 10 seconds
dim = janela  # Dimensão da janela para visualização dos sinais dos indicadores

## ---------------------------------


X = []
Y = []
lances = 0
historico_bid = []
historico_ask = []

historico_lower_band = []
historico_upper_band = []

historico_compras = []
historico_vendas = []
historico_sinal = []

index_compras = []
index_vendas = []

historico = ["","","","","",""]

X_temp = [0,0,0,0,0,0]
epoch = 0


batch = []
compras = [0,0,0,0,0,0]
vendas = [0,0,0,0,0,0]

sinal_action = ['','','','','','']

fig = plt.figure(figsize=(10,10))

ax = fig.gca()

def reset_memoria():
    global historico_compras, historico_vendas, index_compras, index_vendas

    historico_compras = []
    historico_vendas = []
    index_compras = []
    index_vendas = []


def Bollinger_Bands(bid, janela, desvio):
    """desvio: number of std to make the bbands"""

    if len(bid) > janela:
        media = bid.rolling(window= janela).mean()
        rolling_std  = bid.rolling(window= janela).std()
        upper_band = media + (rolling_std * desvio)
        lower_band = media - (rolling_std * desvio)

        #ax.plot(media, '--', color = 'gray', alpha = 0.3)
        ax.plot(upper_band, '--', color = 'green', alpha = 0.5)
        ax.plot(lower_band, '--', color = 'red', alpha = 0.2)

        #ax.scatter(len(ask),media[-1:], color = 'gray', alpha = 0.1)
        ax.scatter(len(bid),upper_band[-1:], color = 'green', alpha = 0.1)
        ax.scatter(len(bid),lower_band[-1:], color = 'red', alpha = 0.1)
        return lower_band, upper_band

    else:
        print("Sem dados suficientes para criar faixas de Bollinger")




def detect_cross(bid, ask, lower_band, upper_band, index):
    """saves the
    historico_compras : (ask price - what price is really paid when u buy)
    index_compras :  (sample index on window)
    historico_vendas :  (bid price - what price is really paid when u sell)
    index_vendas : (sample index on window)
    historico_sinal : [0, 1, 2] hold, buy, sell
    """

    historico_bid.append(bid)
    historico_ask.append(ask)
    historico_lower_band.append(lower_band)
    historico_upper_band.append(upper_band)


    del historico_bid[:-dim]
    del historico_ask[:-dim]
    del historico_lower_band[:-dim]
    del historico_upper_band[:-dim]



    if len(historico_sinal) > 1:

        if historico_bid[-1:] > historico_lower_band[-1:] and historico_bid[-2:-1] <= historico_lower_band[-2:-1]:
            historico_compras.append(float(ask)) # preço de compra
            index_compras.append(index) # indice da amostra na janela
            sinal_action = 1

        elif historico_bid[-1:] < historico_upper_band[-1:] and historico_bid[-2:-1] >= historico_upper_band[-2:-1]:
            historico_vendas.append(float(bid)) # preço de venda
            index_vendas.append(index) # indice da amostra na janela
            sinal_action =2
        else:

            sinal_action = 0
    else:

        sinal_action = 0
    historico_sinal.append(sinal_action)

    return sinal_action


# deals signals on the viewng window!!!
def plota_negociatas(bid, ask, lower_band, upper_band):
    reset_memoria()

    # crossings on the window, for each point on the viewing Window
    # index i, is just the sample position on the window and will be related to the time-frame
    # just fills the historico_compras, index_compras, historico_vendas, index_vendas
    # historico_sigal
    for i in range(len(bid)-(janela), len(bid)):
        sinal_action = detect_cross(float(bid[i]), float(ask[i]), float(lower_band[i]), float(upper_band[i]), i)

    # For plotting porpouses historico_compras, index_compras, historico_vendas, index_vendas?
    if len(historico_compras) > 0:
        ax.scatter(index_compras, historico_compras, marker = 'v', color = "red", label = "Compra")
        for c in range(len(index_compras)):
            ax.text(index_compras[c], historico_compras[c], '- compra', color = "black", alpha = 0.8)

    if len(historico_vendas) > 0:
        ax.scatter(index_vendas, historico_vendas, marker = '^', color = "green", label = "Venda")
        for v in range(len(index_vendas)):
            ax.text(index_vendas[v], historico_vendas[v], '- venda', color = "black", alpha = 0.8)

    return sinal_action

def spread(bid,ask):
    porcento = ask / 100
    diferenca = ask - bid
    porcentagem = diferenca / porcento
    return diferenca, porcentagem


for i in range(10000):

    df = dow[:1079+i].copy()

    if len(df) > 1:
        ax.clear()
        # bid = df['bid']
        # ask = df['ask']
        bid = df.C.reset_index(drop=True) # buy-price it is a Pandas series
        ask = df.C.reset_index(drop=True) # sell-price it is a Pandas series

        # diferenca, porcentagem = spread(bid[-1:],ask[-1:])
        # altough spread is not very usefull coming from bar-history Mt5
        # maybe use tick-data one day to see
        diferenca  = dow.S[-1]
        porcentagem = 100*dow.S[-1]/dow.C[-1] #

        # text information spread in percentage on matplotlib window
        #ax.text(len(bid) + 10, bid[-1:] + (diferenca/2), "Spread " + str(np.around(float(porcentagem),3)) + "%")

        #plt.title("TREINAMENTO - Bitcoin /USD")

        # never price smaller the day except begin of day
        if len(bid) < janela:
            ax.set_xlim(0, len(bid)+(len(bid)/4)+5)
        else:
            ax.set_xlim(len(bid)-janela*2, len(bid)+100)
            bid_min = np.array(bid[-janela:]).min()
            ask_max = np.array(ask[-janela:]).max()
            ax.set_ylim(bid_min-(bid_min * .001),ask_max+(ask_max * .001))

        ax.plot(bid, label = "Bid - Venda BTC (Close Price) "+ str(np.around(float(bid[-1:]),8)), color = 'black', alpha = 0.5)
        # ax.plot(ask, label = "Ask - Compra BTC "+ str(np.around(float(ask[-1:]),8)), color = 'gray', alpha = 0.5)
        plt.legend()

        ax.scatter(len(ask)-1,ask[-1:], color = 'black', alpha = 1)
        ax.scatter(len(bid)-1,bid[-1:], color = 'gray', alpha = 1)
        # more data than the viewing window*3=360*3?
        # data enough for calculating all bbands levels
        if len(bid) > janela * 3:
            # latest variation on price
            bid_mean = float(bid[-1:] / bid[0])
            ask_mean = float(ask[-1:] / ask[0])
            # Calculate 6 bollinger bands (on bid)
            # starting with 0.5*janela= 0.5*360=180
            # increasing so the 6 bollinger bands have lengths
            # 180, 360, 540, 720, 1080
            # For each band get signals of buy/sell or hold
            compensa = 0.5
            for ind in range(0, 6):
                 lower_band, upper_band = Bollinger_Bands(bid, int(janela*compensa), desvio)
                 # calculate all the crossing signals for the entire viewing window
                 # fills historicos (compras, vendas), index (compras, vendas) etc,
                 # historico_sinal filled (mixed bands)
                 # return the last sample on the viewing window (latest) signal
                 sinal_action[ind] = plota_negociatas(bid, ask, lower_band, upper_band)
                 compensa += .5

            # clean up batch-size = 360
            del batch[:-batch_size - 10]
            # put on batch
            # the 6 bollinger bands signals of the latest sample
            batch.append([[sinal_action[0]], [sinal_action[1]],[sinal_action[2]],[sinal_action[3]], [sinal_action[4]], [sinal_action[5]], [bid_mean], [bid_mean]])

            # batch samples > batch_size defined = 360
            # enough samples for a complete batch
            if len(batch) >= batch_size:
                for ind in range(6): # ultima amostra sinais nas 6 bandas
                # historico starts with ["", "", "", "", "", ""]
                # what has happpend in this band previouly
                # compras with [0, 0, 0, 0, 0, 0 ]
                # ONLY BUY POSITIONS, CLOSED BY SELLS SAME SIZE
                # ONLY ONE POSITION OPEN AT TIME!!!!!
                # only sells when said so!
                # THE BUY WILL ONLY BE SOLD WHEN the sold signal
                # happens on the same band
                    if sinal_action[ind] == 1: # ithsmn band signal
                        if historico[ind] != "COMPRA":  # first time yes! # BUY TIME
                            compras[ind] = float(ask[-1:]) # store price of buying
                            X_temp[ind] = batch[-batch_size:] # save the entire batch_size of dimension signals + ask+bid prices
                            print("--**--** COMPRA - ", str(float( compras[ind])))
                            lances += 1
                        elif historico[ind] == "COMPRA":
                            X.append(X_temp[ind])
                            Y.append(0)
                            X_temp[ind] = batch[-batch_size:]
                            compras[ind] = float(ask[-1:])
                            epoch += 1
                        historico[ind] = "COMPRA"
                    if sinal_action[ind] == 2 and historico[ind] == "COMPRA": # Selll what was previouly boght
                        vendas[ind] = float(bid[-1:])
                        epoch += 1
                        lances += 1
                        lucro = float(float(vendas[ind]) - float(compras[ind]))
                        print("--**--** VENDA ", str(float( vendas[ind]))," - Lucro = US$ ", str(lucro))
                        if lucro > 0:
                            try:
                                X.append(X_temp[ind])
                                Y.append(np.array(1))
                                X.append(batch[-batch_size:])
                                Y.append(np.array(2))
                            except:
                                pass
                        if lucro <= 0 or historico[ind] =="VENDA":
                            try:
                                X.append(X_temp[ind])
                                Y.append(np.array(0))
                                X.append(batch[-batch_size:])
                                Y.append(np.array(0))
                            except:
                                pass
                        historico[ind] = "VENDA"
        try:
            X_0 = np.array(X)
            X0 = X_0.reshape(len(Y),-1)
            y = np.array(Y)
        except:
            pass
        # 50 amostras de treino (realmente não da pra ser NN)
        # epohcs may skip rounded numbers, since it can increment
        # from 0 to 6 at each i-step
        if epoch % 50 == 0 and epoch > 0:
            svm.fit(X0, y)
            joblib.dump(svm, "modelo-"+str(epoch)+".pkl", compress=3)
            print("--*--* Modelo Salvo - modelo-"+str(epoch)+".pkl")
        if len(batch) < batch_size:
            print("Batch Total", len(batch))
        print("Epoch - ", str(epoch))

        #time.sleep(intervalo)
        plt.pause(intervalo)
    print("--------------------------- ")
    print("Lances = ", lances)
    print("i: ", i)
